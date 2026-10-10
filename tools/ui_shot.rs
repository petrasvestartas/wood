// A session file rendered with the viewer's own egui interface over it, the layer panel open with every row expanded, without a
// browser or a GPU: a winit window on a virtual X display (Xvfb), Mesa's lavapipe Vulkan, the page's ?cmd= lines run one per frame
// as the web viewer runs them, then the frame (scene + panels) read back with Gpu::render_offscreen.
// The panels (app::ui) compile natively only under cfg(test), so this example is a test; and two native stubs keep them closed, so
// it includes a symlink mirror of src/ (examples/ui_shot_src) with two patched files: app/ui/mod.rs (Ui::publish returns on
// native: it writes the panel state onto the page canvas through web_sys, which panics off wasm) and app/feedback.rs (the
// layer-panel functions use the panel state under cfg(test) instead of the native no-ops). wood/tools/shoot_ui.sh makes the
// mirror, builds and runs this, and converts the PPM:
//   bash wood/tools/shoot_ui.sh <out.png> <scene.pb> [width height]
// by hand, after the mirror exists:
//   Xvfb :99 -screen 0 1920x1200x24 &
//   DISPLAY=:99 VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.json REGEN_PROTO=0 UI_SHOT_IN=<in.pb> UI_SHOT_OUT=<out.ppm> \
//     cargo test --release --example ui_shot --target x86_64-unknown-linux-gnu -- --exact shot::ui_shot --nocapture
// Optional: UI_SHOT_SIZE=1600x1000 (window pixels), UI_SHOT_CMD="Layers All;Element Interactions On;..." (default: the docs line).
#[cfg(not(target_arch = "wasm32"))]
include!("ui_shot_src/lib.rs"); // src/ mirrored by symlinks, app/ui/mod.rs patched: see the header

#[cfg(all(test, not(target_arch = "wasm32")))]
mod shot {
    use crate::app::scene::FileDoc;
    use crate::engine::gpu::FrameInput;
    use crate::State;
    use session_rust::{Session, Xform};
    use std::io::Write;
    use std::rc::Rc;
    use std::sync::Arc;
    use winit::application::ApplicationHandler;
    use winit::event::WindowEvent;
    use winit::event_loop::{ActiveEventLoop, EventLoop};
    use winit::platform::x11::EventLoopBuilderExtX11;
    use winit::window::{Window, WindowId};

    const COMMANDS: &str = "Layers All;Element Interactions On;Element Attributes On;Arctic On;View Isometric;View Orthographic;Fit";

    struct Shot {
        input: String,
        output: String,
        size: (u32, u32),
        error: Option<String>,
    }

    impl ApplicationHandler for Shot {
        fn resumed(&mut self, event_loop: &ActiveEventLoop) {
            if let Err(error) = self.shoot(event_loop) {
                self.error = Some(error.to_string());
            }
            event_loop.exit();
        }

        fn window_event(&mut self, _event_loop: &ActiveEventLoop, _id: WindowId, _event: WindowEvent) {}
    }

    impl Shot {
        fn shoot(&mut self, event_loop: &ActiveEventLoop) -> anyhow::Result<()> {
            // the window and the viewer state on it
            let attributes = Window::default_attributes()
                .with_inner_size(winit::dpi::PhysicalSize::new(self.size.0, self.size.1))
                .with_resizable(false);
            let window = Arc::new(event_loop.create_window(attributes)?);
            let mut state = pollster::block_on(State::new(window.clone(), crate::app::scene::Scene::new()))?;
            state.gpu.view.show_grid = false;

            // the egui panels and their painter
            let mut ui = crate::app::ui::Ui::new(&state.window, state.logical_size()[0]);
            state.gpu.ui = Some(crate::engine::gpu::ui::Ui::new(&state.gpu.ctx, state.gpu.config.format));

            // the scene, as the loader appends a file
            let session = Session::pb_loads(&std::fs::read(&self.input)?).map_err(|e| anyhow::anyhow!(e.to_string()))?;
            let name = std::path::Path::new(&self.input).file_stem().map_or("scene".into(), |s| s.to_string_lossy().into_owned());
            state.append(FileDoc { name, session: Rc::new(session), place: Xform::identity(), point_px: 0.0, display_only: false }, None);
            state.fit_loaded();

            // a few frames so the panels lay out, then one command per frame as ?cmd= runs them
            for _ in 0..3 {
                ui.frame(&mut state);
                state.render();
            }
            let commands = std::env::var("UI_SHOT_CMD").unwrap_or_else(|_| COMMANDS.into());
            for line in commands.split(';').map(str::trim).filter(|line| !line.is_empty()) {
                let message = state.run_command(line).unwrap_or_else(|error| error);
                let rows = crate::app::ui::layers::STATE.with_borrow(|model| model.rows.len());
                eprintln!("{line}: {message} ({rows} layer rows)");
                ui.frame(&mut state);
                state.render();
            }

            // the shading settles over a few frames, then the frame with the panels is read back
            let mut pixels = Vec::new();
            for _ in 0..8 {
                ui.frame(&mut state);
                state.render();
                let rebase = state.gpu.rebase_anchor(&state.camera.origin(), state.camera.distance_world(), crate::engine::performance::now_ms());
                let input = FrameInput {
                    view_proj: state.camera.view_proj_anchored(state.aspect(), &rebase.anchor),
                    clear: wgpu::Color { r: 0.9, g: 0.9, b: 0.9, a: 1.0 },
                    now_ms: crate::engine::performance::now_ms(),
                };
                pixels = state.gpu.render_offscreen(&input);
            }

            let (width, height) = (state.gpu.config.width, state.gpu.config.height);
            let bgra = matches!(state.gpu.config.format, wgpu::TextureFormat::Bgra8Unorm | wgpu::TextureFormat::Bgra8UnormSrgb);
            let mut file = std::fs::File::create(&self.output)?;
            write!(file, "P6\n{width} {height}\n255\n")?;
            for pixel in pixels.chunks_exact(4) {
                let rgb = if bgra { [pixel[2], pixel[1], pixel[0]] } else { [pixel[0], pixel[1], pixel[2]] };
                file.write_all(&rgb)?;
            }
            Ok(())
        }
    }

    #[test]
    fn ui_shot() {
        let size = std::env::var("UI_SHOT_SIZE").ok().and_then(|s| {
            let (w, h) = s.split_once('x')?;
            Some((w.parse().ok()?, h.parse().ok()?))
        });
        let mut shot = Shot {
            input: std::env::var("UI_SHOT_IN").expect("UI_SHOT_IN: the .pb to render"),
            output: std::env::var("UI_SHOT_OUT").expect("UI_SHOT_OUT: the .ppm to write"),
            size: size.unwrap_or((1600, 1000)),
            error: None,
        };
        let event_loop = EventLoop::builder().with_any_thread(true).build().unwrap();
        event_loop.run_app(&mut shot).unwrap();
        if let Some(error) = shot.error {
            panic!("{error}");
        }
    }
}

fn main() {}

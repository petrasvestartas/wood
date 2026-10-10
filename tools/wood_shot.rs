// A session file rendered as the viewer draws it, without a browser: the native headless GPU (any Vulkan, Mesa's lavapipe on a
// machine without a GPU), Arctic shading and black outlines on, the orthographic isometric view fitted to the scene.
//   copied into session_viewer/examples by tools/shoot_native.sh, which builds it with cargo run --release --example wood_shot -- <in.pb> <out.ppm> [width height] [iso|iso_back|top|front]
#[cfg(not(target_arch = "wasm32"))]
include!("../src/lib.rs");

#[cfg(not(target_arch = "wasm32"))]
fn main() -> anyhow::Result<()> {
    use crate::app::scene::{FileDoc, Scene};
    use crate::camera::{Camera, View};
    use engine::gpu::{FrameInput, Gpu};
    use session_rust::{Session, Xform};
    use std::io::Write;
    use std::rc::Rc;

    let args: Vec<String> = std::env::args().collect();
    let (input, output) = (&args[1], &args[2]);
    let width: u32 = args.get(3).and_then(|v| v.parse().ok()).unwrap_or(1600);
    let height: u32 = args.get(4).and_then(|v| v.parse().ok()).unwrap_or(1000);
    let view = match args.get(5).map(String::as_str) {
        Some("iso_back") => View::IsoBack,
        Some("top") => View::Top,
        Some("front") => View::Front,
        _ => View::Iso,
    };

    let session = Session::pb_loads(&std::fs::read(input)?).map_err(|e| anyhow::anyhow!(e.to_string()))?;
    let mut gpu = pollster::block_on(Gpu::new_headless(width, height))?;
    gpu.view.show_grid = false;
    gpu.view.set_arctic(true);

    let mut scene = Scene::new();
    scene.add_file(FileDoc { name: "shot".into(), session: Rc::new(session), place: Xform::identity(), point_px: 0.0, display_only: false });
    let aspect = f64::from(width) / f64::from(height);
    let mut camera = Camera::new();
    camera.set_view(view);
    camera.perspective = false;
    camera.fit(&scene.tables.bounds, aspect);
    scene.upload_to(&mut gpu);
    gpu.find_solids(|row| scene.solid_faces(row));

    // the shading settles over a few frames of the same view, as it does on screen
    let mut pixels = Vec::new();
    for frame in 0..8 {
        let now = 1000.0 + 16.0 * f64::from(frame);
        let rebase = gpu.rebase_anchor(&camera.origin(), camera.distance_world(), now);
        let input = FrameInput {
            view_proj: camera.view_proj_anchored(aspect, &rebase.anchor),
            clear: wgpu::Color { r: 0.9, g: 0.9, b: 0.9, a: 1.0 },
            now_ms: now,
        };
        pixels = gpu.render_offscreen(&input);
    }

    let mut file = std::fs::File::create(output)?;
    write!(file, "P6\n{width} {height}\n255\n")?;
    for pixel in pixels.chunks_exact(4) {
        file.write_all(&pixel[..3])?;
    }
    Ok(())
}

#[cfg(target_arch = "wasm32")]
fn main() {}

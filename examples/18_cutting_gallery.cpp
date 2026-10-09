#include "cutting_gallery.h"
using namespace cutting_gallery;

int main() {
    WoodSession gallery("18_cutting_gallery");
    int column = 0;
    for (const std::string file : {"15_profile_cuts", "16_drill_solids", "17_solid_features"}) {
        WoodSession scene = WoodSession::pb_load(pb_path(file));
        const Xform offset = Xform::translation(column++ * 1100, 0, 0);
        std::map<std::string, std::shared_ptr<TreeNode>> groups;
        for (const std::shared_ptr<Element>& element : *scene.objects.elements) {
            const std::shared_ptr<TreeNode> node = scene.get_node(element->guid());
            const std::shared_ptr<TreeNode> parent = node ? node->parent() : nullptr;
            const std::string label = file + " / " + (parent ? parent->name : "Drill tolerances");
            if (!groups.count(label))
                groups[label] = gallery.add_group(label);
            element->place(offset);
            if (element->name == "Result" || element->name.starts_with("Cutter")) {
                element->compute_geometry_mesh();
                Mesh mesh = element->geometry_mesh();
                mesh.set_objectcolor(element->name == "Result" ? Color(0.24, 0.58, 0.7) : Color(0.9, 0.35, 0.2));
                element->set_geometry(mesh);
            }
            gallery.add(element, groups.at(label));
        }
        std::unordered_set<std::string> copied;
        for (const std::pair<const std::string, std::map<std::string, Edge>>& source : scene.graph.edges)
            for (const std::pair<const std::string, Edge>& target : source.second) {
                const Edge& edge = target.second;
                if (!copied.insert(edge.guid()).second)
                    continue;
                const auto records = scene.interactions.find(edge.guid());
                if (records == scene.interactions.end())
                    continue;
                for (const std::shared_ptr<Interaction>& record : records->second)
                    gallery.Session::add_interaction(gallery.get_element<Element>(source.first), gallery.get_element<Element>(target.first), record);
            }
    }
    finish(gallery, "18_cutting_gallery");
}

/*
directory: cd /home/petras/code/code_cpp/wood_research/wood
run: buildslot ~/.local/bin/cmake --build build --target 18_cutting_gallery --parallel 4 && tools/run_guarded.sh -t 10 -m 4 -- build/18_cutting_gallery
cloudflare: ../bash/publish-scene.sh "$PWD/data/output/pb/18_cutting_gallery.pb" --no-notify
view: https://petrasvestartas.github.io/session/
*/

// The pages as an app: in every section the figures stand beside the source, one sentence says what it is, everything else
// folds under "Notes", and the sections are tabs over the page. Nothing is removed: without this script a page reads in full.
(() => {
  // the landing page's film stands still for those who ask for less motion
  if (matchMedia("(prefers-reduced-motion: reduce)").matches)
    document.querySelectorAll("video.wood-film").forEach(film => { film.removeAttribute("autoplay"); film.load(); });

  // the icons (icons.svg, tools/make_icons.py): a page's by the longest of these its address starts with
  const ICONS = {
    architecture: "architecture", elements: "elements", templates: "diamond_mesh", examples: "install", annotated: "classes", class: "classes", struct: "classes", namespace: "classes",
    elements_node_block: "block", elements_node_support: "support", elements_edge_straight: "edge_straight", elements_edge_curved: "edge_curved",
    elements_edge_variable: "edge_variable", elements_face_planar: "face_planar", elements_face_curved: "face_curved", elements_joint: "insertion_direction",
    elements_joint_cut_plane: "cut_plane", elements_joint_pin: "pin", elements_joint_plate: "joint_type", elements_joint_plate_top_side: "plate_top_side",
    elements_joint_plate_side_side_inplane: "plate_side_side_inplane", elements_joint_plate_side_side_outplane: "plate_side_side_outplane",
    elements_joint_plate_side_side_rotated: "plate_side_side_rotated", elements_joint_plate_cross: "plate_cross", elements_joint_plate_top_top: "plate_top_top",
    elements_joint_plate_border: "plate_border", elements_joint_beam: "beam", elements_joint_beam_cross: "beam_cross", elements_joint_beam_top_side: "beam_top_side",
    elements_joint_beam_top_side_bridle: "beam_bridle", elements_joint_beam_column_head: "beam_column_head", elements_joint_beam_compendium: "beam_compendium",
    elements_joint_library_todo: "beam_todo", elements_joint_beam_keyed_box: "beam_keyed_box", elements_joint_beam_end_to_end: "beam_end_to_end",
    elements_joint_beam_butt: "beam_butt", elements_joint_beam_connectors: "connectors", elements_joint_beam_pins: "beam_pins",
    templates_shells: "translation_shell", templates_folding: "reflex_fold", templates_cylindrical_folding: "cylindrical_folding",
    templates_developable_folding: "developable_folding", templates_chevron: "chevron", templates_cross: "cross", templates_module: "module",
    templates_reciprocal: "reciprocal_rotation", templates_grid: "grid", templates_floor: "floor", templates_truss: "truss", templates_vault: "vault",
    templates_plaited: "plaited", templates_vidy: "vidy",
    "#assembly_sequence": "insertion_direction", "#reflex_fold": "reflex_fold", "#diamond_mesh": "diamond_mesh", "#reciprocal_move": "reciprocal_move", "#reciprocal_rotation": "reciprocal_rotation",
    "#ex_two_plates": "plate_top_side", "#ex_2_contacts": "joint_type", "#ex_3_features": "solver", "#ex_4_traversal": "architecture", "#ex_5_settings": "diamond_mesh",
    "#ex_6_custom_joint": "plate_side_side_inplane", "#ex_7_assignment": "insertion_direction", "#ex_8_beams": "beam_cross", "#ex_9_cross_joints": "plate_cross",
    "#ex_10_profile_cuts": "cut_plane", "#ex_11_drill_solids": "pin", "#ex_12_solid_features": "block",
  };
  const iconOf = address => {
    const last = (address || "").split("/").pop().replace(/\.html$/, "").replace(/^#.*_assembly_sequence$/, "#assembly_sequence");
    const key = Object.keys(ICONS).filter(name => last === name || (!name.startsWith("#") && !last.includes("#") && last.startsWith(name))).sort((x, y) => y.length - x.length)[0];
    if (!key)
      return null;
    const icon = document.createElementNS("http://www.w3.org/2000/svg", "svg");
    const use = document.createElementNS("http://www.w3.org/2000/svg", "use");
    icon.setAttribute("class", "wood-icon");
    icon.setAttribute("aria-hidden", "true");
    use.setAttribute("href", "icons.svg#" + ICONS[key]);
    icon.append(use);
    return icon;
  };
  const give = (element, address) => {
    const icon = element && !element.querySelector(".wood-icon") && iconOf(address);
    if (icon)
      element.prepend(icon);
  };

  // in the bar, beside the page's title and on the cards; in the tree as it is built, its own pages only
  for (const link of document.querySelectorAll("#chapters a"))
    give(link, link.getAttribute("href"));
  if (!/^(class|struct|namespace)/.test(location.pathname.split("/").pop()))
    give(document.querySelector("div.header .title"), location.pathname);
  for (const card of document.querySelectorAll("a.wood-card")) {
    const title = card.querySelector(".wood-card-title");
    title.textContent = title.textContent.trim().replace(/^./, letter => letter.toUpperCase());
    give(title, card.getAttribute("href"));
  }
  const tree = document.getElementById("nav-tree-contents");
  if (tree) {
    const dress = () => tree.querySelectorAll(".label a[href]").forEach(link => {
      if (/^(architecture|elements|templates|examples|annotated)/.test(link.getAttribute("href")))
        give(link, link.getAttribute("href"));
    });
    new MutationObserver(dress).observe(tree, { childList: true, subtree: true });
    dress();
  }

  // the chapter this page belongs to, in the bar
  const page = location.pathname.split("/").pop();
  for (const link of document.querySelectorAll("#chapters a")) {
    const chapter = link.getAttribute("href").split("/").pop().replace(".html", "");
    if (page.startsWith(chapter) || (chapter === "annotated" && /^(class|struct|namespace|hierarchy|functions)/.test(page)))
      link.classList.add("on");
  }

  const block = document.querySelector("div.contents > div.textblock");
  if (!block || block.querySelector(".wood-hero") || document.querySelector("table.memberdecls, table.directory"))
    return;

  const make = (tag, name, ...children) => {
    const element = document.createElement(tag);
    element.className = name;
    element.append(...children);
    return element;
  };
  const heading = node => node.matches && node.matches("h1.doxsection, h2.doxsection, h3.doxsection, h4.doxsection");
  const textless = node => !node.textContent.trim();

  // where the first sentence of a paragraph ends: a full stop before a capital letter or the end, never inside code
  const sentenceEnd = paragraph => {
    const walker = document.createTreeWalker(paragraph, NodeFilter.SHOW_TEXT);
    for (let text = walker.nextNode(); text; text = walker.nextNode()) {
      if (text.parentElement.closest("code, span.tt, a"))
        continue;
      const match = text.data.match(/[.](?=\s+[A-Z(])|[.:]\s*$/);
      if (match)
        return [text, match.index];
    }
    return null;
  };

  // a paragraph's first sentence, the rest handed to the notes; a long one is cut at two lines and kept whole in the notes
  const lead = (paragraph, notes) => {
    const end = sentenceEnd(paragraph);
    const whole = paragraph.cloneNode(true);
    if (end) {
      const rest = document.createRange();
      rest.setStart(end[0], end[1] + (end[0].data[end[1]] === ":" ? 0 : 1));
      rest.setEndAfter(paragraph.lastChild);
      const more = rest.extractContents();
      if (end[0].data.endsWith(":"))
        end[0].data = end[0].data.replace(/\s*:\s*$/, ".");
      if (more.textContent.replace(/[.:\s]/g, ""))
        notes.unshift(whole.textContent.length > 0 ? whole : more);
    }
    paragraph.classList.add("wood-lead");
    if (paragraph.textContent.length > 170) {
      paragraph.classList.add("wood-cut");
      if (!notes.includes(whole))
        notes.unshift(whole);
    }
    return paragraph;
  };

  // a code block that scrolls in its own window, with a button that opens it whole
  const source = code => {
    const open = make("button", "wood-expand", "Expand");
    const frame = make("div", "wood-source", code, open);
    open.type = "button";
    open.addEventListener("click", () => { open.textContent = frame.classList.toggle("open") ? "Collapse" : "Expand"; });
    requestAnimationFrame(() => { if (code.scrollHeight <= code.clientHeight + 4) open.remove(); });
    return frame;
  };

  // the joint library's rows (a title, then GIF, still and tile) as cards: the GIF, the title under it, the tile beside the title
  const cards = table => {
    const grid = make("div", "wood-joint-grid");
    let title = "";
    for (const row of table.querySelectorAll("tr")) {
      const [gif, , tile] = [...row.cells].map(cell => cell.querySelector("img"));
      if (gif) {
        const name = make("span", "wood-joint-name", title);
        const card = make("figure", "wood-joint", gif, make("figcaption", "", name));
        if (tile)
          card.lastChild.append(tile);
        grid.append(card);
      } else if (row.querySelector("td")) {
        title = row.textContent.trim().replace(/,\s*p\.\s*(\d+)$/, ", p. $1");
      }
    }
    return grid;
  };

  const section = nodes => {
    const out = make("section", "wood-section");
    const figures = [], codes = [], wide = [], notes = [], links = [], kept = [];
    let first = null;
    for (const node of nodes) {
      if (node.nodeType !== Node.ELEMENT_NODE) {
        if (!textless(node)) kept.push(node);
      } else if (heading(node)) {
        out.append(node);
      } else if (node.matches("div.fragment, pre.fragment, pre:not(.mermaid)")) {
        codes.push(node);
      } else if (node.matches("div.wood-card-grid, div[style*='display:none']")) {
        wide.push(node);
      } else if (node.matches("div.image, div.wood-pair-grid, pre.mermaid") || (node.matches("p") && node.querySelector("img") && textless(node))) {
        figures.push(node);
      } else if (node.querySelector && node.querySelectorAll("td:first-child img[src$='.gif']").length > 8 && node.querySelector("td:nth-child(2) img[src$='.png']")) {
        wide.push(cards(node));
      } else if (node.querySelector && node.querySelector("img.inline")) {
        (nodes.some(other => other.matches && other.matches("div.fragment")) ? figures : wide).push(node);
      } else if (node.matches("p") && node.querySelector("span[style^='color:']")) {
        node.classList.add("wood-legend");
        links.push(node);
      } else if (node.matches("p") && /^(Code|Example|Step \d+)\b/.test(node.textContent.trim())) {
        node.classList.add("wood-code-link");
        links.push(node);
      } else if (node.matches("p") && textless(node)) {
        kept.push(node);
      } else if (node.matches("p") && !first) {
        first = node;
      } else if (node.matches("p, ul, ol, dl, blockquote, table, div, em")) {
        notes.push(node);
      } else {
        kept.push(node);
      }
    }
    if (first)
      out.append(lead(first, notes));
    out.append(...kept.filter(node => node.nodeType !== Node.ELEMENT_NODE || !node.matches("hr")));

    const shown = make("div", "wood-figures", ...figures);
    if (figures.length > 1)
      shown.classList.add("many");
    if (figures.length && codes.length)
      out.append(make("div", "wood-panes", shown, make("div", "wood-codes", ...codes.map(source))));
    else if (figures.length)
      out.append(shown);
    else
      out.append(...codes.map(source));

    out.append(...links, ...wide);
    if (notes.length) {
      const tables = notes.every(node => node.matches("table"));
      out.append(make("details", "wood-notes", make("summary", "", tables ? "Table" : "Notes"), ...notes));
    }
    return out;
  };

  // the sections, one per heading, what comes before the first heading one of its own
  const groups = [[]];
  for (const node of [...block.childNodes]) {
    if (heading(node))
      groups.push([]);
    groups[groups.length - 1].push(node);
  }
  block.replaceChildren(...groups.filter(nodes => nodes.length).map(section));
  document.documentElement.classList.add("wood-app");

  // the tabs: the page's main sections, the one in view marked
  const titles = [...block.querySelectorAll("h1.doxsection")];
  if (titles.length > 1) {
    const tabs = make("nav", "wood-tabs");
    tabs.setAttribute("aria-label", "Sections");
    const tab = new Map(titles.map(title => {
      const link = make("a", "", title.textContent.trim());
      link.href = "#" + (title.querySelector("a.anchor") || title).id;
      tabs.append(link);
      return [title, link];
    }));
    block.prepend(tabs);
    const scroller = document.getElementById("doc-content");
    const mark = () => {
      const passed = titles.filter(title => title.getBoundingClientRect().top < scroller.getBoundingClientRect().top + 160);
      const current = tab.get(passed[passed.length - 1] || titles[0]);
      if (current.classList.contains("on"))
        return;
      tabs.querySelectorAll(".on").forEach(link => link.classList.remove("on"));
      current.classList.add("on");
      tabs.scrollTo({ left: current.offsetLeft - tabs.clientWidth / 2 + current.clientWidth / 2, behavior: "smooth" });
    };
    scroller.addEventListener("scroll", mark, { passive: true });
    mark();
  }
})();

// The pages as an app: in every section the figures stand beside the source, one sentence says what it is, everything else
// folds under "Notes", and the sections are tabs over the page. Nothing is removed: without this script a page reads in full.
(() => {
  // the landing page's film stands still for those who ask for less motion
  if (matchMedia("(prefers-reduced-motion: reduce)").matches)
    document.querySelectorAll("video.wood-film").forEach(film => { film.removeAttribute("autoplay"); film.load(); });

  const ORDER = "architecture beam beam_bridle beam_butt beam_column_head beam_compendium beam_cross beam_end_to_end beam_keyed_box beam_pins beam_todo beam_top_side block chevron classes connectors cross cut_plane cylindrical_folding developable_folding edge_curved edge_straight edge_variable elements examples face_curved face_planar floor grid insertion_direction joint_type module pin plaited plate_border plate_cross plate_side_side_inplane plate_side_side_outplane plate_side_side_rotated plate_top_side plate_top_top reciprocal_move reciprocal_rotation reflex_fold solver support templates translation_shell truss vault vidy".split(" "); // of icons.webp, 8 a row, written by make_icons.py
  // the icons (icons.webp, tools/make_icons.py): a page's by the longest of these its address starts with
  const ICONS = {
    architecture: "architecture", elements: "elements", templates: "templates", examples: "examples", annotated: "classes", class: "classes", struct: "classes", namespace: "classes",
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
    "#assembly_sequence": "insertion_direction", "#reflex_fold": "reflex_fold", "#diamond_mesh": "templates", "#reciprocal_move": "reciprocal_move", "#reciprocal_rotation": "reciprocal_rotation",
    "#ex_two_plates": "plate_top_side", "#ex_2_contacts": "joint_type", "#ex_3_features": "solver", "#ex_4_traversal": "architecture", "#ex_5_settings": "beam_todo",
    "#ex_6_custom_joint": "plate_side_side_inplane", "#ex_7_assignment": "insertion_direction", "#ex_8_beams": "beam_cross", "#ex_9_cross_joints": "plate_cross",
    "#ex_10_profile_cuts": "cut_plane", "#ex_11_drill_solids": "pin", "#ex_12_solid_features": "block",
  };
  const iconOf = address => {
    const last = (address || "").split("/").pop().replace(/\.html$/, "").replace(/^#.*_assembly_sequence$/, "#assembly_sequence");
    const key = Object.keys(ICONS).filter(name => last === name || (!name.startsWith("#") && !last.includes("#") && last.startsWith(name))).sort((x, y) => y.length - x.length)[0];
    if (!key)
      return null;
    const icon = document.createElement("span");
    const place = ORDER.indexOf(ICONS[key]);
    icon.className = "wood-icon";
    icon.setAttribute("aria-hidden", "true");
    icon.style.backgroundPosition = `${place % 8 * 100 / 7}% ${Math.floor(place / 8) * 100 / (Math.ceil(ORDER.length / 8) - 1)}%`;
    icon.style.backgroundSize = `800% ${Math.ceil(ORDER.length / 8) * 100}%`;
    return icon;
  };
  const give = (element, address) => {
    const icon = element && !element.querySelector(".wood-icon") && iconOf(address);
    if (icon)
      element.prepend(icon);
  };

  // beside the page's title and on the cards
  const page = location.pathname.split("/").pop() || "index.html";
  if (!/^(class|struct|namespace)/.test(page))
    give(document.querySelector("div.header .title"), location.pathname);
  for (const card of document.querySelectorAll("a.wood-card")) {
    const title = card.querySelector(".wood-card-title");
    title.textContent = title.textContent.trim().replace(/^./, letter => letter.toUpperCase());
    give(title, card.getAttribute("href"));
  }

  // the list at the side: the pages of the chapter, the Elements by what they are, from the data of doxygen's own tree; a
  // chapter of one page has none. On a phone the button in the bar opens it, the chapters over it.
  const side = document.createElement("nav");
  side.className = "wood-side";
  side.setAttribute("aria-label", "Pages");
  for (const link of document.querySelectorAll("#chapters a")) {
    const chapter = link.cloneNode(true);
    chapter.className = "wood-side-chapter";
    side.append(chapter);
  }
  document.body.append(side);
  const GROUPS = [["elements_node", "Node"], ["elements_edge", "Edge"], ["elements_face", "Face"], ["elements_joint_plate", "Plate joints"], ["elements_joint_beam", "Beam joints"], ["elements_joint_library", "Beam joints"], ["elements_joint", "Joint"]];
  const SHORT = /^(Node|Edge|Face|Plate|Beam|Joint) (?=.)/;
  const load = name => new Promise(done => {
    const script = document.createElement("script");
    script.src = name + ".js";
    script.onload = () => done(window[name]);
    script.onerror = () => done([]);
    document.head.append(script);
  });
  const entry = ([name, address], inner) => {
    const link = document.createElement("a");
    link.href = address;
    link.textContent = address.startsWith("elements_") ? name.replace(SHORT, "").replace(/ Joints$/, name === "Plate Joints" || name === "Beam Joints" ? "" : "$&").replace(/^(Joints|Joint)$/, "Overview").replace("Joint library: what is left", "Library, what is left") : name;
    if (inner)
      link.className = "inner";
    else
      give(link, address);
    if (address === page)
      link.classList.add("on");
    return link;
  };
  const isPage = ([, address]) => address && !address.includes("#");
  const list = async chapter => {
    const pages = (await load(chapter)).filter(isPage);
    if (chapter === "elements") {
      const named = new Map();
      for (const item of pages) {
        const group = GROUPS.find(([start]) => item[1].startsWith(start));
        if (group)
          named.set(group[1], [...(named.get(group[1]) || []), item]);
      }
      for (const name of ["Node", "Edge", "Face", "Joint", "Plate joints", "Beam joints"]) {
        const label = document.createElement("span");
        label.textContent = name;
        const links = (named.get(name) || []).map(item => entry(item));
        side.append(label, ...links.filter(link => link.textContent === "Overview"), ...links.filter(link => link.textContent !== "Overview"));
      }
    } else {
      for (const item of pages) {
        side.append(entry(item));
        // the pages under the one that is open
        let under = item;
        while (page.startsWith(item[1].replace(".html", "")) && typeof under[2] === "string") {
          const deeper = (await load(under[2])).filter(isPage);
          side.append(...deeper.map(child => entry(child, true)));
          under = deeper.find(child => typeof child[2] === "string") || [];
        }
      }
    }
    const here = side.querySelector("a.on:not(.wood-side-chapter)");
    if (here && here.offsetTop > side.clientHeight - 80)
      side.scrollTop = here.offsetTop - side.clientHeight / 2;
  };
  const chapter = ["elements", "templates"].find(name => page.startsWith(name));
  if (chapter) {
    document.documentElement.classList.add("wood-sided");
    list(chapter);
  }

  // the chapter this page belongs to, in the bar
  for (const link of document.querySelectorAll("#chapters a, .wood-side-chapter")) {
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
  const heading = node => node.matches && node.matches("h1, h2, h3, h4");
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

  // a paragraph's first sentence, the paragraph itself handed to the notes when it says more; a sentence of more than two
  // lines goes to the notes whole
  const lead = (paragraph, notes) => {
    const whole = paragraph.cloneNode(true);
    const end = sentenceEnd(paragraph);
    if (end) {
      const rest = document.createRange();
      rest.setStart(end[0], end[1] + (end[0].data[end[1]] === ":" ? 0 : 1));
      rest.setEndAfter(paragraph.lastChild);
      const more = rest.extractContents();
      if (end[0].data.endsWith(":"))
        end[0].data = end[0].data.replace(/\s*:\s*$/, ".");
      if (more.textContent.replace(/[.:\s]/g, ""))
        notes.unshift(whole);
    }
    if (paragraph.textContent.trim().length > 150) {
      if (!notes.includes(whole))
        notes.unshift(whole);
      return null;
    }
    paragraph.classList.add("wood-lead");
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
      } else if (node.matches("p") && /^Step \d+ of/.test(node.textContent.trim())) {
        continue; // the list at the side leads on
      } else if (node.matches("p") && node.querySelector("span[style^='color:']")) {
        node.classList.add("wood-legend");
        links.push(node);
      } else if (node.matches("p") && node.querySelector("a") && node.textContent.length < 220 && /^(Code|Example)\b/.test(node.textContent.trim())) {
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
    const said = first && lead(first, notes);
    if (said)
      out.append(said);
    out.append(...kept.filter(node => node.nodeType !== Node.ELEMENT_NODE || !node.matches("hr")));

    if (out.children.length === 1 && heading(out.firstChild) && !figures.length && !codes.length && !wide.length && !notes.length && !links.length)
      out.classList.add("wood-title-only");
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
  if (groups.length > 1 && groups[0].every(node => textless(node) && !(node.querySelector && node.querySelector("img"))))
    groups[1].push(...groups.shift());
  block.replaceChildren(...groups.filter(nodes => nodes.length).map(section));
  document.documentElement.classList.add("wood-app");

  // the floor film's frame numbers stay in the sources
  for (const title of block.querySelectorAll("h1, h2"))
    for (const text of title.childNodes)
      if (text.nodeType === Node.TEXT_NODE)
        text.data = text.data.replace(/^(\s*)\d{3}\. /, "$1");

  // the tabs: the page's main sections, the one in view marked
  const titles = [...block.querySelectorAll("h1.doxsection, .wood-section > h1")];
  if (titles.length > 1 && page !== "elements.html") {
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

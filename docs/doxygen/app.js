// The pages as an app: in every section the figures stand beside the source, one sentence says what it is, everything else
// folds under "Notes", and the sections are tabs over the page. Nothing is removed: without this script a page reads in full.
(() => {
  // the landing page's film stands still for those who ask for less motion
  if (matchMedia("(prefers-reduced-motion: reduce)").matches)
    document.querySelectorAll("video.wood-film").forEach(film => { film.removeAttribute("autoplay"); film.load(); });

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
      } else if (node.querySelector && node.querySelector("img.inline")) {
        (node.querySelectorAll("tr").length > 4 ? wide : figures).push(node);
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

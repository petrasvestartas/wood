<template>
  <div class="test-layout">
    <main class="test-main">
      <table v-if="groupedTests.length">
        <thead>
          <tr>
            <th>
              <a href="https://github.com/petrasvestartas/wood" target="_blank" class="lang-link" title="Wood C++">
                <svg class="lang-icon" viewBox="0 0 24 24" fill="currentColor">
                  <path d="M12 0C5.37 0 0 5.37 0 12c0 5.31 3.435 9.795 8.205 11.385.6.105.825-.255.825-.57 0-.285-.015-1.23-.015-2.235-3.015.555-3.795-.735-4.035-1.41-.135-.345-.72-1.41-1.23-1.695-.42-.225-1.02-.78-.015-.795.945-.015 1.62.87 1.845 1.23 1.08 1.815 2.805 1.305 3.495.99.105-.78.42-1.305.765-1.605-2.67-.3-5.46-1.335-5.46-5.925 0-1.305.465-2.385 1.23-3.225-.12-.3-.54-1.53.12-3.18 0 0 1.005-.315 3.3 1.23.96-.27 1.98-.405 3-.405s2.04.135 3 .405c2.295-1.56 3.3-1.23 3.3-1.23.66 1.65.24 2.88.12 3.18.765.84 1.23 1.905 1.23 3.225 0 4.605-2.805 5.625-5.475 5.925.435.375.81 1.095.81 2.22 0 1.605-.015 2.895-.015 3.3 0 .315.225.69.825.57A12.02 12.02 0 0024 12c0-6.63-5.37-12-12-12z"/>
                </svg>
              </a>
            </th>
          </tr>
        </thead>
        <tbody>
          <template v-for="g in groupedTests" :key="g.name">
            <tr class="test-name-row" :id="'test-' + g.name">
              <td>
                <strong>{{ g.name }}</strong>
              </td>
            </tr>
            <tr>
              <!-- C++ column -->
              <td class="lang-col">
                <div v-if="g.cpp" class="test-card">
                  <div :class="['tag', g.cpp.passed ? 'tag-pass' : 'tag-fail']">
                    <i :class="g.cpp.passed ? 'fa-solid fa-check' : 'fa-solid fa-xmark'"></i> {{ formatTime(g.cpp.time_ms) }} ms
                  </div>
                  <div v-if="g.cpp.code" class="code-shell">
                    <button
                      class="code-copy-btn"
                      type="button"
                      @click="copyCode(g.cpp)"
                      title="Copy code"
                      aria-label="Copy code"
                    >
                    </button>
                    <div v-html="highlightedCode(g.cpp)"></div>
                  </div>
                  <div class="failures" v-if="!g.cpp.passed">
                    <div><strong>Failing checks:</strong></div>
                    <ul>
                      <li v-for="c in failingChecks(g.cpp)" :key="'cpp-' + g.name + ':' + c.line">
                        line {{ c.line }}: <span class="inline-code" v-html="highlightedCheck(c)"></span>
                      </li>
                    </ul>

                    <div v-if="hasFailures(g.cpp)" class="exceptions">
                      <div><strong>Errors / Exceptions:</strong></div>
                      <ul>
                        <li
                          v-for="f in errorFailures(g.cpp)"
                          :key="'cpp-err-' + g.name + ':' + (f.line || 0)"
                        >
                          <div v-if="f.file">at {{ f.file }}<span v-if="f.line">:{{ f.line }}</span></div>
                          <div v-if="f.code_line">
                            <span class="inline-code" v-html="highlightedCheck(f)"></span>
                          </div>
                          <div class="error-message" v-if="f.error">{{ f.error }}</div>
                        </li>
                      </ul>
                    </div>
                  </div>
                </div>
                <div v-else class="missing">–</div>
              </td>
            </tr>
          </template>
        </tbody>
      </table>

      <div v-else>
        No test results loaded yet. Run <code>./bash/wood_test.sh</code> to generate test data.
      </div>
    </main>
  </div>
</template>

<script setup>
import { computed, ref, onMounted } from 'vue'
import Parser from 'web-tree-sitter'

const props = defineProps({
  tests: { type: Array, required: true },
  activeSuite: { type: String, required: true }
})

defineEmits(['update:activeSuite'])

// Tree-sitter state
let parser = null
const languages = {}
const queries = {}
const ready = ref(false)

const BASE = import.meta.env.BASE_URL || '/'
const WASM_PATHS = {
  cpp: `${BASE}tree-sitter-cpp.wasm`,
}

const QUERIES_MAP = {
  cpp: `
    (comment) @comment
    (number_literal) @number
    (string_literal) @string
    (raw_string_literal) @string
    (char_literal) @string
    (system_lib_string) @string
    (type_identifier) @type
    (primitive_type) @type.builtin
    (sized_type_specifier) @type.builtin
    (namespace_identifier) @module
  `,
}

const CSS = {
  'comment': 'ts-c', 'number': 'ts-n', 'string': 'ts-s',
  'type': 'ts-ty', 'type.builtin': 'ts-tyb', 'type.def': 'ts-tyd',
  'property': 'ts-pr', 'module': 'ts-mod', 'macro': 'ts-mc',
  'constant.builtin': 'ts-cb', 'decorator': 'ts-dec',
  'function': 'ts-fn', 'function.def': 'ts-fnd', 'method': 'ts-mt',
  'keyword': 'ts-kw', 'variable': 'ts-v', 'variable.builtin': 'ts-vb',
  'parameter': 'ts-pm', 'operator': 'ts-op',
  'punctuation.bracket': 'ts-pb', 'punctuation.delimiter': 'ts-pd',
}

const escapeHtml = (str) => {
  return str.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;')
}

const CPP_KEYWORDS = new Set(['if','else','for','while','do','return','class','struct','enum','namespace','using','template','typename','public','private','protected','virtual','const','static','inline','new','delete','try','catch','throw','void','int','double','float','bool','char','auto','sizeof','constexpr','override','explicit','extern','volatile','mutable','friend','operator','switch','case','default','break','continue','typedef','union','noexcept','nullptr','true','false','this','#include','#define','#ifdef','#ifndef','#endif','#if','co_await','co_return','co_yield','concept','requires','static_assert','static_cast','dynamic_cast','reinterpret_cast','const_cast'])

const highlightGap = (text) => {
  const kw = CPP_KEYWORDS
  const re = /(#?\w+)|([(){}\[\]])|([,;])|([+\-*/%=!<>&|^~?:.@#]+)|(\s+)/g
  const tokens = []
  let m
  while ((m = re.exec(text)) !== null) {
    tokens.push({ text: m[0], word: m[1], bracket: m[2], delim: m[3], op: m[4], ws: m[5] })
  }
  let result = ''
  for (let i = 0; i < tokens.length; i++) {
    const t = tokens[i]
    if (t.ws) { result += t.ws; continue }
    if (t.bracket) { result += `<span class="ts-pb">${escapeHtml(t.bracket)}</span>`; continue }
    if (t.delim) { result += `<span class="ts-pd">${escapeHtml(t.delim)}</span>`; continue }
    if (t.op) { result += `<span class="ts-op">${escapeHtml(t.op)}</span>`; continue }
    if (t.word) {
      let nextSym = null
      for (let j = i + 1; j < tokens.length; j++) {
        if (!tokens[j].ws) { nextSym = tokens[j]; break }
      }
      const followedByParen = nextSym && nextSym.bracket === '('
      let prevSym = null
      for (let j = i - 1; j >= 0; j--) {
        if (!tokens[j].ws) { prevSym = tokens[j]; break }
      }
      const afterDot = prevSym && prevSym.op && /^(::|\.|->) *$/.test(prevSym.op)
      let prevWord = null
      for (let j = i - 1; j >= 0; j--) {
        if (tokens[j].word) { prevWord = tokens[j].word; break }
        if (!tokens[j].ws) break
      }
      const MODULE_KW = new Set(['from','import','use','mod','crate','namespace','using','package'])
      const afterModuleKw = prevWord && MODULE_KW.has(prevWord)
      const isPascal = /^[A-Z][a-zA-Z0-9]+$/.test(t.word)

      if (kw.has(t.word)) {
        result += `<span class="ts-kw">${escapeHtml(t.word)}</span>`
      } else if (afterModuleKw && !followedByParen) {
        result += `<span class="ts-mod">${escapeHtml(t.word)}</span>`
      } else if (isPascal) {
        result += `<span class="ts-ty">${escapeHtml(t.word)}</span>`
      } else if (followedByParen && afterDot) {
        result += `<span class="ts-mt">${escapeHtml(t.word)}</span>`
      } else if (followedByParen) {
        result += `<span class="ts-fn">${escapeHtml(t.word)}</span>`
      } else if (/^[A-Z][A-Z0-9_]+$/.test(t.word)) {
        result += `<span class="ts-cb">${escapeHtml(t.word)}</span>`
      } else {
        result += escapeHtml(t.word)
      }
      continue
    }
    result += escapeHtml(t.text)
  }
  return result
}

const highlight = (code) => {
  if (!parser || !languages['cpp']) return escapeHtml(code)
  try {
    parser.setLanguage(languages['cpp'])
    const tree = parser.parse(code)
    if (!tree) return escapeHtml(code)

    const query = queries['cpp']
    if (!query) { tree.delete(); return escapeHtml(code) }

    const captures = query.captures(tree.rootNode)
    const best = new Map()
    for (const cap of captures) {
      const s = cap.node.startIndex
      const e = cap.node.endIndex
      if (s === e) continue
      const key = `${s}:${e}`
      const existing = best.get(key)
      if (!existing || cap.name.split('.').length >= existing.name.split('.').length) {
        best.set(key, cap)
      }
    }

    const intervals = Array.from(best.values())
      .map(c => ({ s: c.node.startIndex, e: c.node.endIndex, cls: CSS[c.name] || 'ts-v' }))
      .sort((a, b) => a.s - b.s || (a.e - a.s) - (b.e - b.s))

    let result = ''
    let pos = 0
    for (const iv of intervals) {
      if (iv.s < pos) continue
      if (iv.s > pos) result += highlightGap(code.slice(pos, iv.s))
      const text = code.slice(iv.s, iv.e)
      result += `<span class="${iv.cls}">${escapeHtml(text)}</span>`
      pos = iv.e
    }
    if (pos < code.length) result += highlightGap(code.slice(pos))

    tree.delete()
    return result
  } catch (e) {
    console.error('highlight error', e)
    return escapeHtml(code)
  }
}

onMounted(async () => {
  try {
    await Parser.init({ locateFile: (f) => `${BASE}${f}` })
    parser = new Parser()
  } catch (e) {
    console.error('tree-sitter init failed:', e)
    return
  }
  try {
    const language = await Parser.Language.load(WASM_PATHS.cpp)
    languages['cpp'] = language
    queries['cpp'] = language.query(QUERIES_MAP.cpp)
  } catch (e) {
    console.error('tree-sitter cpp query failed:', e.message)
  }
  ready.value = true
})

const groupedTests = computed(() => {
  const byName = new Map()
  for (const t of props.tests) {
    if (t.suite !== props.activeSuite) continue
    const name = t.test_name || "(unnamed)"
    if (!byName.has(name)) {
      byName.set(name, { name, cpp: null })
    }
    const entry = byName.get(name)
    if (t.language === "cpp") entry.cpp = t
  }
  return Array.from(byName.values())
})

const formatTime = (time_ms) => {
  return typeof time_ms === 'number' && time_ms.toFixed ? time_ms.toFixed(3) : time_ms
}

const normalizeForDisplay = (code) => {
  if (!code) return ""
  const lines = code.split('\n').map((line) => {
    const m = line.match(/^(\s*)\/\/\s*uncomment\s+(.*)$/)
    if (m) return m[1] + m[2]
    return line
  })
  let minIndent = Infinity
  for (const line of lines) {
    if (!line.trim()) continue
    const m = line.match(/^(\s*)/)
    const indent = m ? m[1].length : 0
    if (indent < minIndent) minIndent = indent
  }
  if (!Number.isFinite(minIndent) || minIndent === 0) return lines.join('\n').replace(/(\n\s*)+$/, '')
  return lines.map((line) => (line.length >= minIndent ? line.slice(minIndent) : line)).join('\n').replace(/(\n\s*)+$/, '')
}

const highlightedCode = (t) => {
  if (!t || !t.code) return ""
  const code = normalizeForDisplay(t.code)
  if (!ready.value) return `<pre><code>${escapeHtml(code)}</code></pre>`
  return `<pre><code>${highlight(code)}</code></pre>`
}

const highlightedCheck = (check) => {
  if (!check || !check.code_line) return ""
  if (!ready.value) return escapeHtml(check.code_line)
  return highlight(check.code_line)
}

const failingChecks = (t) => {
  if (!t.checks) return []
  return t.checks.filter(c => c && c.passed === false)
}

const hasFailures = (t) => {
  return !!(t && Array.isArray(t.failures) && t.failures.length > 0)
}

const errorFailures = (t) => {
  if (!t || !Array.isArray(t.failures)) return []
  return t.failures
}

const copyCode = (t) => {
  if (!t || !t.code) return
  const text = normalizeForDisplay(t.code)
  try {
    if (typeof navigator !== 'undefined' && navigator.clipboard && navigator.clipboard.writeText) {
      navigator.clipboard.writeText(text)
    }
  } catch (e) { /* ignore */ }
}
</script>

<style scoped>
.test-layout {
  display: flex;
  height: 100%;
}
.test-main {
  flex: 1;
  overflow-y: auto;
}

table {
  width: 100%;
  border-collapse: collapse;
  background: #000000;
  box-shadow: none;
  table-layout: fixed;
}
th, td {
  padding: 0.5rem 0.75rem;
  border: none;
  vertical-align: top;
  color: #aaaaaa;
}
th {
  background: #000000;
  text-align: left;
  font-weight: 600;
  font-size: 14px;
  color: #ffffff;
  border: none;
}

.lang-icon {
  width: 24px;
  height: 24px;
  color: #ffffff;
}

.lang-link {
  color: #ffffff;
  text-decoration: none;
  transition: color 0.2s;
  display: inline-flex;
  align-items: center;
}

.lang-link:hover {
  color: #aaaaaa;
}

.tag {
  display: inline-block;
  padding: 0.2rem 0;
  font-weight: 600;
  font-size: 0.85rem;
  background: none;
  border: none;
}
.tag-pass {
  color: #50fa7b;
}
.tag-fail {
  color: #ff5555;
}
pre {
  margin: 0;
  background: transparent;
  border-radius: 0;
  padding: 0;
}
.test-card :deep(pre code) {
  white-space: pre-wrap;
  word-wrap: break-word;
  overflow-wrap: anywhere;
  color: #abb2bf;
}
.code-shell {
  position: relative;
  margin: 0.25rem 0 0.5rem 0;
  background: #0f0f0f;
  border-radius: 4px;
  border: none;
}

.code-shell :deep(pre) {
  margin: 0;
  padding: 0.75rem;
  background: transparent !important;
}
.code-shell :deep(code) {
  white-space: pre-wrap;
  word-wrap: break-word;
  overflow-wrap: anywhere;
}

.code-copy-btn {
  position: absolute;
  top: 4px;
  right: 4px;
  width: 12px;
  height: 12px;
  padding: 0;
  border: none;
  border-radius: 50%;
  background: #444444;
  cursor: pointer;
}

.code-copy-btn:hover {
  background: #666666;
}
.failures {
  margin-top: 0.35rem;
  color: #ff5555;
}
.exceptions {
  margin-top: 0.35rem;
}
.error-message {
  margin-top: 0.15rem;
  font-family: monospace;
  color: #ff5555;
}
.test-card {
  display: flex;
  flex-direction: column;
  gap: 0.25rem;
}
.test-card .tag {
  align-self: flex-end;
}
.missing {
  font-size: 0.8rem;
  color: #666666;
  font-style: italic;
}
.test-name-row td {
  padding-top: 0.75rem;
  font-weight: 600;
  color: #ffffff;
  background: #000000;
}
.lang-col {
  width: 100%;
}
</style>

<style>
/* Tree-sitter highlight theme */
.ts-kw  { color: #c678dd; }
.ts-dir { color: #c678dd; }
.ts-ty  { color: #00e5ff; }
.ts-tyb { color: #56b6c2; }
.ts-tyd { color: #00e5ff; font-weight: 600; }
.ts-fn  { color: #61afef; }
.ts-fnd { color: #61afef; font-weight: 600; }
.ts-mt  { color: #61afef; }
.ts-mc  { color: #61afef; font-weight: 600; }
.ts-v   { color: #abb2bf; }
.ts-vb  { color: #e06c75; font-style: italic; }
.ts-pm  { color: #e5e54b; font-style: italic; }
.ts-pl  { color: #e5e54b; }
.ts-pr  { color: #e06c75; }
.ts-cb  { color: #e5e54b; }
.ts-s   { color: #98c379; }
.ts-n   { color: #e5e54b; }
.ts-c   { color: #5c6370; font-style: italic; }
.ts-op  { color: #56b6c2; }
.ts-mod { color: #00e5ff; }
.ts-lb  { color: #00e5ff; font-style: italic; }
.ts-dec { color: #00e5ff; }
.ts-pb  { color: #ff79c6; }
.ts-pd  { color: #ff79c6; }
.inline-code pre { display: inline; margin: 0; padding: 0; }
.inline-code code { display: inline; }
</style>

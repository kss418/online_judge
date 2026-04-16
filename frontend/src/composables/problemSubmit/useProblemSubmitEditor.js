import { computed, nextTick, watch } from 'vue'

function buildEditorPlaceholder(activeLanguage){
  if (!activeLanguage) {
    return '// 언어를 선택하면 예시 형식이 바뀝니다.'
  }

  const languageName = String(activeLanguage.language || '').toLowerCase()
  const sourceExtension = String(activeLanguage.source_extension || '').toLowerCase()

  if (languageName.includes('cpp') || sourceExtension === '.cpp') {
    return [
      '#include <bits/stdc++.h>',
      '',
      'using namespace std;',
      '',
      'int main(){',
      '    ios::sync_with_stdio(false);',
      '    cin.tie(nullptr);',
      '',
      '    return 0;',
      '}'
    ].join('\n')
  }

  if (languageName.includes('python') || sourceExtension === '.py') {
    return [
      'import sys',
      '',
      'def main():',
      '    input = sys.stdin.readline',
      '',
      '',
      "if __name__ == '__main__':",
      '    main()'
    ].join('\n')
  }

  if (languageName.includes('java') || sourceExtension === '.java') {
    return [
      'import java.io.*;',
      '',
      'public class Main {',
      '    public static void main(String[] args) throws Exception {',
      '    }',
      '}'
    ].join('\n')
  }

  return '// 여기에 코드를 작성하세요.'
}

export function useProblemSubmitEditor({
  selectedLanguage,
  sourceCode,
  sourceEditorElement,
  supportedLanguages,
  isSubmittingSubmission,
  onSubmit
}){
  const activeLanguage = computed(() =>
    supportedLanguages.value.find((language) => language.language === selectedLanguage.value) || null
  )
  const editorLineCount = computed(() =>
    Math.max(1, sourceCode.value.split('\n').length)
  )
  const editorLineNumbers = computed(() =>
    Array.from({ length: editorLineCount.value }, (_, index) => index + 1)
  )
  const editorPlaceholder = computed(() => buildEditorPlaceholder(activeLanguage.value))

  watch(sourceCode, () => {
    scheduleEditorResize()
  })

  watch(supportedLanguages, (languages) => {
    if (!languages.length) {
      selectedLanguage.value = ''
      return
    }

    const hasSelectedLanguage = languages.some(
      (language) => language.language === selectedLanguage.value
    )

    if (!hasSelectedLanguage) {
      selectedLanguage.value = languages[0].language
    }
  }, {
    immediate: true
  })

  function scheduleEditorResize(){
    nextTick(() => {
      resizeEditorHeight()
    })
  }

  function resizeEditorHeight(){
    if (!sourceEditorElement.value) {
      return
    }

    sourceEditorElement.value.style.height = 'auto'
    sourceEditorElement.value.style.height = `${Math.max(sourceEditorElement.value.scrollHeight, 320)}px`
  }

  function handleSourceEditorInput(){
    scheduleEditorResize()
  }

  function handleEditorKeydown(event){
    if ((event.ctrlKey || event.metaKey) && event.key === 'Enter') {
      event.preventDefault()
      void onSubmit()
      return
    }

    if (event.key !== 'Tab' || !sourceEditorElement.value || isSubmittingSubmission.value) {
      return
    }

    event.preventDefault()

    const textarea = sourceEditorElement.value
    const selectionStart = textarea.selectionStart
    const selectionEnd = textarea.selectionEnd

    if (event.shiftKey) {
      outdentEditorSelection(selectionStart, selectionEnd)
      return
    }

    indentEditorSelection(selectionStart, selectionEnd)
  }

  function indentEditorSelection(selectionStart, selectionEnd){
    const indentText = '    '

    if (selectionStart === selectionEnd) {
      const nextValue =
        `${sourceCode.value.slice(0, selectionStart)}${indentText}${sourceCode.value.slice(selectionEnd)}`
      updateEditorValue(
        nextValue,
        selectionStart + indentText.length,
        selectionStart + indentText.length
      )
      return
    }

    const blockRange = getSelectedLineRange(sourceCode.value, selectionStart, selectionEnd)
    const selectedBlock = sourceCode.value.slice(blockRange.lineStart, blockRange.lineEnd)
    const selectedLines = selectedBlock.split('\n')
    const indentedBlock = selectedLines
      .map((line) => `${indentText}${line}`)
      .join('\n')

    const nextValue =
      `${sourceCode.value.slice(0, blockRange.lineStart)}${indentedBlock}${sourceCode.value.slice(blockRange.lineEnd)}`

    updateEditorValue(
      nextValue,
      selectionStart + indentText.length,
      selectionEnd + (indentText.length * selectedLines.length)
    )
  }

  function outdentEditorSelection(selectionStart, selectionEnd){
    const blockRange = getSelectedLineRange(sourceCode.value, selectionStart, selectionEnd)
    const selectedBlock = sourceCode.value.slice(blockRange.lineStart, blockRange.lineEnd)
    const selectedLines = selectedBlock.split('\n')
    const removedIndentSizes = selectedLines.map((line) => getOutdentWidth(line))
    const totalRemovedIndent = removedIndentSizes.reduce((sum, width) => sum + width, 0)

    if (!totalRemovedIndent) {
      return
    }

    const outdentedBlock = selectedLines
      .map((line, index) => line.slice(removedIndentSizes[index]))
      .join('\n')

    const nextValue =
      `${sourceCode.value.slice(0, blockRange.lineStart)}${outdentedBlock}${sourceCode.value.slice(blockRange.lineEnd)}`

    const removedBeforeSelectionStart =
      selectionStart > blockRange.lineStart ? removedIndentSizes[0] : 0
    const nextSelectionStart =
      selectionStart === selectionEnd
        ? Math.max(blockRange.lineStart, selectionStart - removedIndentSizes[0])
        : Math.max(blockRange.lineStart, selectionStart - removedBeforeSelectionStart)
    const nextSelectionEnd = Math.max(nextSelectionStart, selectionEnd - totalRemovedIndent)

    updateEditorValue(
      nextValue,
      nextSelectionStart,
      nextSelectionEnd
    )
  }

  function getSelectedLineRange(value, selectionStart, selectionEnd){
    const lineStart = value.lastIndexOf('\n', Math.max(0, selectionStart - 1)) + 1
    const normalizedSelectionEnd =
      selectionEnd > selectionStart && value[selectionEnd - 1] === '\n'
        ? selectionEnd - 1
        : selectionEnd
    const lineEnd = value.indexOf('\n', normalizedSelectionEnd)

    return {
      lineStart,
      lineEnd: lineEnd === -1 ? value.length : lineEnd
    }
  }

  function getOutdentWidth(line){
    if (line.startsWith('\t')) {
      return 1
    }

    let leadingSpaceCount = 0
    while (leadingSpaceCount < 4 && line[leadingSpaceCount] === ' ') {
      leadingSpaceCount += 1
    }
    return leadingSpaceCount
  }

  function updateEditorValue(nextValue, selectionStart, selectionEnd){
    sourceCode.value = nextValue

    nextTick(() => {
      if (!sourceEditorElement.value) {
        return
      }

      sourceEditorElement.value.focus()
      sourceEditorElement.value.setSelectionRange(selectionStart, selectionEnd)
      resizeEditorHeight()
    })
  }

  function resetSourceCode(){
    sourceCode.value = ''
    scheduleEditorResize()
  }

  return {
    activeLanguage,
    editorLineNumbers,
    editorPlaceholder,
    scheduleEditorResize,
    handleSourceEditorInput,
    handleEditorKeydown,
    resetSourceCode
  }
}

import { computed, ref } from 'vue'

export function useTestcaseZipInput({
  onInvalidZip,
  onValidZip
} = {}){
  const testcaseZipFile = ref(null)
  const testcaseZipInputKey = ref(0)

  const selectedTestcaseZipName = computed(() => testcaseZipFile.value?.name || '')

  function resetTestcaseZipSelection(){
    testcaseZipFile.value = null
    testcaseZipInputKey.value += 1
  }

  function handleTestcaseZipFileChange(event){
    const nextFile = event.target?.files?.[0] || null
    if (!nextFile) {
      testcaseZipFile.value = null
      return
    }

    if (!nextFile.name.toLowerCase().endsWith('.zip')) {
      resetTestcaseZipSelection()
      onInvalidZip?.()
      return
    }

    testcaseZipFile.value = nextFile
    onValidZip?.(nextFile)
  }

  return {
    testcaseZipFile,
    testcaseZipInputKey,
    selectedTestcaseZipName,
    resetTestcaseZipSelection,
    handleTestcaseZipFileChange
  }
}

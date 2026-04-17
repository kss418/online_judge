import { computed, ref, watch } from 'vue'

import { useTestcaseZipInput } from '@/composables/adminShared/useTestcaseZipInput'

export function useTestcaseDraftState({
  authState,
  busySection,
  selectedTestcaseResource,
  showErrorNotice
}){
  const newTestcaseInput = ref('')
  const newTestcaseOutput = ref('')
  const selectedTestcaseInputDraft = ref('')
  const selectedTestcaseOutputDraft = ref('')
  const {
    testcaseZipFile,
    testcaseZipInputKey,
    selectedTestcaseZipName,
    resetTestcaseZipSelection,
    handleTestcaseZipFileChange
  } = useTestcaseZipInput({
    onInvalidZip(){
      showErrorNotice('ZIP 파일만 업로드할 수 있습니다.')
    }
  })
  const canSaveSelectedTestcase = computed(() => {
    if (!selectedTestcaseResource.selectedTestcase.value || !authState.token || busySection.value) {
      return false
    }

    return (
      selectedTestcaseInputDraft.value !== selectedTestcaseResource.selectedTestcase.value.testcase_input ||
      selectedTestcaseOutputDraft.value !== selectedTestcaseResource.selectedTestcase.value.testcase_output
    )
  })

  function resetDrafts(){
    newTestcaseInput.value = ''
    newTestcaseOutput.value = ''
    selectedTestcaseInputDraft.value = ''
    selectedTestcaseOutputDraft.value = ''
    resetTestcaseZipSelection()
  }

  function resetZipSelection(){
    resetTestcaseZipSelection()
  }

  watch(selectedTestcaseResource.selectedTestcase, (testcase) => {
    selectedTestcaseInputDraft.value = testcase?.testcase_input ?? ''
    selectedTestcaseOutputDraft.value = testcase?.testcase_output ?? ''
  }, {
    immediate: true
  })

  return {
    newTestcaseInput,
    newTestcaseOutput,
    selectedTestcaseInputDraft,
    selectedTestcaseOutputDraft,
    testcaseZipFile,
    testcaseZipInputKey,
    selectedTestcaseZipName,
    canSaveSelectedTestcase,
    resetDrafts,
    resetZipSelection,
    handleTestcaseZipFileChange
  }
}

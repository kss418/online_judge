import { computed, ref } from 'vue'

import {
  isProblemSampleBusySection,
  makeProblemSampleBusySection
} from '@/composables/adminProblems/problemBusySection'
import {
  makeSampleDraft
} from '@/composables/adminProblems/problemHelpers'
import { parsePositiveInteger } from '@/utils/parse'

export function useProblemEditorDraft({
  authState,
  busySection,
  getSelectedProblemDetail,
  setActionFeedback
}){
  const titleDraft = ref('')
  const timeLimitDraft = ref('')
  const memoryLimitDraft = ref('')
  const descriptionDraft = ref('')
  const inputFormatDraft = ref('')
  const outputFormatDraft = ref('')
  const noteDraft = ref('')
  const sampleDrafts = ref([])
  const testcaseZipFile = ref(null)
  const testcaseZipInputKey = ref(0)

  const selectedTestcaseZipName = computed(() => testcaseZipFile.value?.name || '')
  const canSaveTitle = computed(() => {
    const selectedProblemDetail = getSelectedProblemDetail()
    if (!selectedProblemDetail || !authState.token || busySection.value) {
      return false
    }

    const nextTitle = titleDraft.value.trim()
    return Boolean(nextTitle) && nextTitle !== selectedProblemDetail.title
  })
  const canSaveLimits = computed(() => {
    const selectedProblemDetail = getSelectedProblemDetail()
    if (!selectedProblemDetail || !authState.token || busySection.value) {
      return false
    }

    const nextTimeLimit = parsePositiveInteger(timeLimitDraft.value)
    const nextMemoryLimit = parsePositiveInteger(memoryLimitDraft.value)

    if (nextTimeLimit == null || nextMemoryLimit == null) {
      return false
    }

    return (
      nextTimeLimit !== selectedProblemDetail.limits.time_limit_ms ||
      nextMemoryLimit !== selectedProblemDetail.limits.memory_limit_mb
    )
  })
  const canSaveStatement = computed(() => {
    const selectedProblemDetail = getSelectedProblemDetail()
    if (!selectedProblemDetail || !authState.token || busySection.value) {
      return false
    }

    if (
      !descriptionDraft.value.trim() ||
      !inputFormatDraft.value.trim() ||
      !outputFormatDraft.value.trim()
    ) {
      return false
    }

    return (
      descriptionDraft.value !== selectedProblemDetail.statement.description ||
      inputFormatDraft.value !== selectedProblemDetail.statement.input_format ||
      outputFormatDraft.value !== selectedProblemDetail.statement.output_format ||
      noteDraft.value !== selectedProblemDetail.statement.note
    )
  })

  function resetEditorDrafts(){
    titleDraft.value = ''
    timeLimitDraft.value = ''
    memoryLimitDraft.value = ''
    descriptionDraft.value = ''
    inputFormatDraft.value = ''
    outputFormatDraft.value = ''
    noteDraft.value = ''
    sampleDrafts.value = []
    testcaseZipFile.value = null
    testcaseZipInputKey.value += 1
  }

  function assignEditorDrafts(problemDetail){
    titleDraft.value = problemDetail.title
    timeLimitDraft.value = String(problemDetail.limits.time_limit_ms || '')
    memoryLimitDraft.value = String(problemDetail.limits.memory_limit_mb || '')
    descriptionDraft.value = problemDetail.statement.description
    inputFormatDraft.value = problemDetail.statement.input_format
    outputFormatDraft.value = problemDetail.statement.output_format
    noteDraft.value = problemDetail.statement.note
    sampleDrafts.value = problemDetail.samples.map(makeSampleDraft)
  }

  function syncSampleDrafts(samples){
    sampleDrafts.value = (Array.isArray(samples) ? samples : []).map(makeSampleDraft)
  }

  function getSelectedProblemSample(sampleOrder){
    return getSelectedProblemDetail()?.samples.find((sample) => sample.sample_order === sampleOrder) || null
  }

  function getSampleDraft(sampleOrder){
    return sampleDrafts.value.find((sample) => sample.sample_order === sampleOrder) || null
  }

  function isSavingSample(sampleOrder){
    return isProblemSampleBusySection(busySection.value, sampleOrder)
  }

  function canSaveSample(sampleOrder){
    const selectedProblemDetail = getSelectedProblemDetail()
    if (!selectedProblemDetail || !authState.token || busySection.value) {
      return false
    }

    const selectedSample = getSelectedProblemSample(sampleOrder)
    const sampleDraft = getSampleDraft(sampleOrder)
    if (!selectedSample || !sampleDraft) {
      return false
    }

    return (
      sampleDraft.sample_input !== selectedSample.sample_input ||
      sampleDraft.sample_output !== selectedSample.sample_output
    )
  }

  function isLastSample(sampleOrder){
    const samples = getSelectedProblemDetail()?.samples || []
    if (!samples.length) {
      return false
    }

    return samples[samples.length - 1].sample_order === sampleOrder
  }

  function handleTestcaseZipFileChange(event){
    const nextFile = event.target?.files?.[0] || null
    if (!nextFile) {
      testcaseZipFile.value = null
      return
    }

    if (!nextFile.name.toLowerCase().endsWith('.zip')) {
      testcaseZipFile.value = null
      testcaseZipInputKey.value += 1
      setActionFeedback({
        message: '',
        error: 'ZIP 파일만 업로드할 수 있습니다.'
      })
      return
    }

    testcaseZipFile.value = nextFile
    setActionFeedback({
      error: ''
    })
  }

  return {
    titleDraft,
    timeLimitDraft,
    memoryLimitDraft,
    descriptionDraft,
    inputFormatDraft,
    outputFormatDraft,
    noteDraft,
    sampleDrafts,
    testcaseZipFile,
    testcaseZipInputKey,
    selectedTestcaseZipName,
    canSaveTitle,
    canSaveLimits,
    canSaveStatement,
    resetEditorDrafts,
    assignEditorDrafts,
    syncSampleDrafts,
    getSampleDraft,
    isSavingSample,
    canSaveSample,
    isLastSample,
    handleTestcaseZipFileChange
  }
}

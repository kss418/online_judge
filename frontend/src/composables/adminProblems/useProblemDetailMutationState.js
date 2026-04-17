import { computed, ref } from 'vue'

import { runBusyAction } from '@/composables/adminShared/runBusyAction'
import { useTestcaseZipInput } from '@/composables/adminShared/useTestcaseZipInput'
import { useTestcaseZipUploadAction } from '@/composables/adminShared/useTestcaseZipUploadAction'
import {
  isProblemSampleBusySection,
  makeProblemSampleBusySection,
  problemBusySection
} from '@/composables/adminProblems/problemBusySection'
import { makeSampleDraft } from '@/composables/adminProblems/problemHelpers'
import {
  updateProblemLimits,
  updateProblemStatement,
  updateProblemTitle
} from '@/api/problemAdminApi'
import {
  createProblemSample,
  deleteProblemSample,
  updateProblemSample
} from '@/api/problemSampleApi'
import { formatApiError } from '@/utils/apiError'
import { parsePositiveInteger } from '@/utils/parse'

function normalizeProblemMutationError(error, fallbackError){
  return formatApiError(error, {
    fallback: fallbackError
  })
}

export function useProblemDetailMutationState({
  authState,
  busySection,
  formatCount,
  selectedProblemDetail,
  problemDetailResource,
  problemCatalogResource,
  loadSelectedProblem,
  showSuccessNotice,
  showErrorNotice
}){
  const titleDraft = ref('')
  const timeLimitDraft = ref('')
  const memoryLimitDraft = ref('')
  const descriptionDraft = ref('')
  const inputFormatDraft = ref('')
  const outputFormatDraft = ref('')
  const noteDraft = ref('')
  const sampleDrafts = ref([])
  const {
    testcaseZipFile,
    testcaseZipInputKey,
    selectedTestcaseZipName,
    resetTestcaseZipSelection,
    handleTestcaseZipFileChange
  } = useTestcaseZipInput({
    onInvalidZip(){
      showErrorNotice('ZIP 파일만 업로드할 수 있습니다.', {
        duration: 5000
      })
    }
  })

  const isSavingTitle = computed(() => busySection.value === problemBusySection.SAVE_TITLE)
  const isSavingLimits = computed(() => busySection.value === problemBusySection.SAVE_LIMITS)
  const isSavingStatement = computed(() => busySection.value === problemBusySection.SAVE_STATEMENT)
  const isCreatingSample = computed(() => busySection.value === problemBusySection.CREATE_SAMPLE)
  const isDeletingLastSample = computed(() => busySection.value === problemBusySection.DELETE_LAST_SAMPLE)
  const selectedProblemId = computed(() => Number(selectedProblemDetail.value?.problem_id ?? 0))

  const canSaveTitle = computed(() => {
    const problemDetail = selectedProblemDetail.value
    if (!problemDetail || !authState.token || busySection.value) {
      return false
    }

    const nextTitle = titleDraft.value.trim()
    return Boolean(nextTitle) && nextTitle !== problemDetail.title
  })
  const canSaveLimits = computed(() => {
    const problemDetail = selectedProblemDetail.value
    if (!problemDetail || !authState.token || busySection.value) {
      return false
    }

    const nextTimeLimit = parsePositiveInteger(timeLimitDraft.value)
    const nextMemoryLimit = parsePositiveInteger(memoryLimitDraft.value)

    if (nextTimeLimit == null || nextMemoryLimit == null) {
      return false
    }

    return (
      nextTimeLimit !== problemDetail.limits.time_limit_ms ||
      nextMemoryLimit !== problemDetail.limits.memory_limit_mb
    )
  })
  const canSaveStatement = computed(() => {
    const problemDetail = selectedProblemDetail.value
    if (!problemDetail || !authState.token || busySection.value) {
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
      descriptionDraft.value !== problemDetail.statement.description ||
      inputFormatDraft.value !== problemDetail.statement.input_format ||
      outputFormatDraft.value !== problemDetail.statement.output_format ||
      noteDraft.value !== problemDetail.statement.note
    )
  })
  const canCreateSample = computed(() =>
    Boolean(selectedProblemDetail.value) &&
    Boolean(authState.token) &&
    !busySection.value
  )
  const canDeleteLastSample = computed(() =>
    Boolean(selectedProblemDetail.value?.samples.length) &&
    Boolean(authState.token) &&
    !busySection.value
  )

  function assignSamples(samples){
    sampleDrafts.value = (Array.isArray(samples) ? samples : []).map(makeSampleDraft)
  }

  function reset(options = {}){
    titleDraft.value = ''
    timeLimitDraft.value = ''
    memoryLimitDraft.value = ''
    descriptionDraft.value = ''
    inputFormatDraft.value = ''
    outputFormatDraft.value = ''
    noteDraft.value = ''
    sampleDrafts.value = []

    if (!options.skipTestcaseZipReset) {
      resetTestcaseZipSelection()
    }
  }

  function assignFromProblemDetail(problemDetail){
    titleDraft.value = problemDetail.title
    timeLimitDraft.value = String(problemDetail.limits.time_limit_ms || '')
    memoryLimitDraft.value = String(problemDetail.limits.memory_limit_mb || '')
    descriptionDraft.value = problemDetail.statement.description
    inputFormatDraft.value = problemDetail.statement.input_format
    outputFormatDraft.value = problemDetail.statement.output_format
    noteDraft.value = problemDetail.statement.note
    assignSamples(problemDetail.samples)
  }

  function getSelectedProblemSample(sampleOrder){
    return selectedProblemDetail.value?.samples.find((sample) => sample.sample_order === sampleOrder) || null
  }

  function getSampleDraft(sampleOrder){
    return sampleDrafts.value.find((sample) => sample.sample_order === sampleOrder) || null
  }

  function isSavingSample(sampleOrder){
    return isProblemSampleBusySection(busySection.value, sampleOrder)
  }

  function canSaveSample(sampleOrder){
    const problemDetail = selectedProblemDetail.value
    if (!problemDetail || !authState.token || busySection.value) {
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
    const samples = selectedProblemDetail.value?.samples || []
    if (!samples.length) {
      return false
    }

    return samples[samples.length - 1].sample_order === sampleOrder
  }

  async function runProblemMutation({
    section,
    canRun,
    runRequest,
    patchDetail,
    patchSummary,
    afterSuccess,
    successMessage,
    fallbackError
  }){
    const problemDetail = selectedProblemDetail.value

    if (!authState.token || !problemDetail || !canRun) {
      return
    }

    const problemId = problemDetail.problem_id

    return runBusyAction({
      busySection,
      section,
      run: async () => {
        const response = await runRequest(problemId)

        if (typeof patchDetail === 'function') {
          problemDetailResource.setSelectedProblemDetail((currentProblemDetail) =>
            patchDetail(currentProblemDetail, response)
          )
        }

        if (typeof afterSuccess === 'function') {
          afterSuccess(problemId, response)
        }

        problemDetailResource.applyProblemVersion(problemId, response.version)

        if (patchSummary) {
          problemCatalogResource.mergeProblemSummary(problemId, patchSummary)
        }

        showSuccessNotice(successMessage(problemId, response))
      },
      onError: (error) => {
        showErrorNotice(normalizeProblemMutationError(error, fallbackError), {
          duration: 5000
        })
      }
    })
  }

  async function handleSaveTitle(){
    const nextTitle = titleDraft.value.trim()

    return runProblemMutation({
      section: problemBusySection.SAVE_TITLE,
      canRun: canSaveTitle.value,
      runRequest(problemId){
        return updateProblemTitle(problemId, {
          title: nextTitle
        }, authState.token)
      },
      patchDetail(problemDetail){
        return problemDetail
          ? {
            ...problemDetail,
            title: nextTitle
          }
          : problemDetail
      },
      patchSummary: {
        title: nextTitle
      },
      afterSuccess(){
        titleDraft.value = nextTitle
      },
      successMessage(problemId){
        return `문제 #${formatCount(problemId)} 제목을 저장했습니다.`
      },
      fallbackError: '문제 제목을 저장하지 못했습니다.'
    })
  }

  async function handleSaveLimits(){
    const nextTimeLimit = parsePositiveInteger(timeLimitDraft.value)
    const nextMemoryLimit = parsePositiveInteger(memoryLimitDraft.value)

    if (nextTimeLimit == null || nextMemoryLimit == null) {
      return
    }

    return runProblemMutation({
      section: problemBusySection.SAVE_LIMITS,
      canRun: canSaveLimits.value,
      runRequest(problemId){
        return updateProblemLimits(problemId, {
          time_limit_ms: nextTimeLimit,
          memory_limit_mb: nextMemoryLimit
        }, authState.token)
      },
      patchDetail(problemDetail){
        return problemDetail
          ? {
            ...problemDetail,
            limits: {
              time_limit_ms: nextTimeLimit,
              memory_limit_mb: nextMemoryLimit
            }
          }
          : problemDetail
      },
      patchSummary: {
        time_limit_ms: nextTimeLimit,
        memory_limit_mb: nextMemoryLimit
      },
      afterSuccess(){
        timeLimitDraft.value = String(nextTimeLimit)
        memoryLimitDraft.value = String(nextMemoryLimit)
      },
      successMessage(problemId){
        return `문제 #${formatCount(problemId)} 제한을 저장했습니다.`
      },
      fallbackError: '문제 제한을 저장하지 못했습니다.'
    })
  }

  async function handleSaveStatement(){
    const nextStatement = {
      description: descriptionDraft.value,
      input_format: inputFormatDraft.value,
      output_format: outputFormatDraft.value,
      note: noteDraft.value === '' ? null : noteDraft.value
    }

    return runProblemMutation({
      section: problemBusySection.SAVE_STATEMENT,
      canRun: canSaveStatement.value,
      runRequest(problemId){
        return updateProblemStatement(problemId, nextStatement, authState.token)
      },
      patchDetail(problemDetail){
        return problemDetail
          ? {
            ...problemDetail,
            statement: {
              description: descriptionDraft.value,
              input_format: inputFormatDraft.value,
              output_format: outputFormatDraft.value,
              note: noteDraft.value
            }
          }
          : problemDetail
      },
      successMessage(problemId){
        return `문제 #${formatCount(problemId)} 설명을 저장했습니다.`
      },
      fallbackError: '문제 설명을 저장하지 못했습니다.'
    })
  }

  async function handleCreateSample(){
    if (!authState.token || !selectedProblemDetail.value || !canCreateSample.value) {
      return
    }

    const problemId = selectedProblemDetail.value.problem_id

    return runBusyAction({
      busySection,
      section: problemBusySection.CREATE_SAMPLE,
      run: async () => {
        const response = await createProblemSample(problemId, authState.token)
        const nextSampleOrder = Number(response.sample_order ?? selectedProblemDetail.value.samples.length + 1)
        const nextSamples = [
          ...selectedProblemDetail.value.samples,
          {
            sample_order: nextSampleOrder,
            sample_input: '',
            sample_output: ''
          }
        ]

        problemDetailResource.setSelectedProblemSamples(nextSamples)
        assignSamples(nextSamples)
        problemDetailResource.applyProblemVersion(problemId, response.version)
        showSuccessNotice(`예제 ${formatCount(nextSampleOrder)}를 추가했습니다.`)
      },
      onError: (error) => {
        showErrorNotice(normalizeProblemMutationError(error, '공개 예제를 추가하지 못했습니다.'), {
          duration: 5000
        })
      }
    })
  }

  async function handleSaveSample(sampleOrder){
    if (!authState.token || !selectedProblemDetail.value || !canSaveSample(sampleOrder)) {
      return
    }

    const problemId = selectedProblemDetail.value.problem_id
    const sampleDraft = getSampleDraft(sampleOrder)
    if (!sampleDraft) {
      return
    }

    return runBusyAction({
      busySection,
      section: makeProblemSampleBusySection(sampleOrder),
      run: async () => {
        const response = await updateProblemSample(problemId, sampleOrder, {
          sample_input: sampleDraft.sample_input,
          sample_output: sampleDraft.sample_output
        }, authState.token)
        const nextSamples = selectedProblemDetail.value.samples.map((sample) =>
          sample.sample_order === sampleOrder
            ? {
              ...sample,
              sample_input: response.sample_input ?? sampleDraft.sample_input,
              sample_output: response.sample_output ?? sampleDraft.sample_output
            }
            : sample
        )

        problemDetailResource.setSelectedProblemSamples(nextSamples)
        assignSamples(nextSamples)
        problemDetailResource.applyProblemVersion(problemId, response.version)
        showSuccessNotice(`예제 ${formatCount(sampleOrder)}를 저장했습니다.`)
      },
      onError: (error) => {
        showErrorNotice(normalizeProblemMutationError(error, '공개 예제를 저장하지 못했습니다.'), {
          duration: 5000
        })
      }
    })
  }

  async function handleDeleteLastSample(){
    if (!authState.token || !selectedProblemDetail.value || !canDeleteLastSample.value) {
      return
    }

    const problemId = selectedProblemDetail.value.problem_id
    const lastSample = selectedProblemDetail.value.samples[selectedProblemDetail.value.samples.length - 1]
    if (!lastSample) {
      return
    }

    return runBusyAction({
      busySection,
      section: problemBusySection.DELETE_LAST_SAMPLE,
      run: async () => {
        const response = await deleteProblemSample(problemId, authState.token)
        const nextSamples = selectedProblemDetail.value.samples.filter(
          (sample) => sample.sample_order !== lastSample.sample_order
        )

        problemDetailResource.setSelectedProblemSamples(nextSamples)
        assignSamples(nextSamples)
        problemDetailResource.applyProblemVersion(problemId, response.version)
        showSuccessNotice(`예제 ${formatCount(lastSample.sample_order)}를 삭제했습니다.`)
      },
      onError: (error) => {
        showErrorNotice(normalizeProblemMutationError(error, '공개 예제를 삭제하지 못했습니다.'), {
          duration: 5000
        })
      }
    })
  }

  const testcaseZipUploadAction = useTestcaseZipUploadAction({
    authState,
    busySection,
    uploadSection: problemBusySection.UPLOAD_TESTCASE_ZIP,
    selectedProblemId,
    testcaseZipFile,
    resetTestcaseZipSelection,
    async afterUpload(response, problemId){
      problemDetailResource.applyProblemVersion(problemId, response.version)
      await loadSelectedProblem(problemId, {
        skipTestcaseZipReset: true
      })
    },
    showSuccess(message){
      showSuccessNotice(message)
    },
    showError(error){
      showErrorNotice(error, {
        duration: 5000
      })
    },
    formatSuccessMessage(response){
      const testcaseCount = Number(response.testcase_count ?? 0)
      return `비공개 테스트케이스 ${formatCount(testcaseCount)}개를 업로드했습니다.`
    },
    fallbackError: '비공개 테스트케이스 ZIP을 업로드하지 못했습니다.'
  })

  return {
    titleDraft,
    timeLimitDraft,
    memoryLimitDraft,
    descriptionDraft,
    inputFormatDraft,
    outputFormatDraft,
    noteDraft,
    sampleDrafts,
    testcaseZipInputKey,
    selectedTestcaseZipName,
    canSaveTitle,
    canSaveLimits,
    canSaveStatement,
    canCreateSample,
    canDeleteLastSample,
    canUploadTestcaseZip: testcaseZipUploadAction.canUploadTestcaseZip,
    isSavingTitle,
    isSavingLimits,
    isSavingStatement,
    isCreatingSample,
    isDeletingLastSample,
    isUploadingTestcaseZip: testcaseZipUploadAction.isUploadingTestcaseZip,
    reset,
    assignFromProblemDetail,
    getSampleDraft,
    isSavingSample,
    canSaveSample,
    isLastSample,
    handleSaveTitle,
    handleSaveLimits,
    handleSaveStatement,
    handleCreateSample,
    handleSaveSample,
    handleDeleteLastSample,
    handleTestcaseZipFileChange,
    handleUploadTestcaseZip: testcaseZipUploadAction.handleUploadTestcaseZip
  }
}

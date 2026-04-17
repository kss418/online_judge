import { computed } from 'vue'

import { runBusyAction } from '@/composables/adminShared/runBusyAction'
import { useTestcaseZipUploadAction } from '@/composables/adminShared/useTestcaseZipUploadAction'
import {
  makeProblemSampleBusySection,
  problemBusySection
} from '@/composables/adminProblems/problemBusySection'
import {
  createProblemSample,
  deleteProblemSample,
  updateProblemSample
} from '@/api/problemSampleApi'
import {
  updateProblemLimits,
  updateProblemStatement,
  updateProblemTitle
} from '@/api/problemAdminApi'
import { formatApiError } from '@/utils/apiError'
import { parsePositiveInteger } from '@/utils/parse'

export function useProblemDetailEditorActions({
  authState,
  busySection,
  formatCount,
  editorDraft,
  problemActionFeedback,
  problemDetailResource,
  problemCatalogResource,
  loadSelectedProblem
}){
  const selectedProblemDetail = problemDetailResource.selectedProblemDetail
  const isSavingTitle = computed(() => busySection.value === problemBusySection.SAVE_TITLE)
  const isSavingLimits = computed(() => busySection.value === problemBusySection.SAVE_LIMITS)
  const isSavingStatement = computed(() => busySection.value === problemBusySection.SAVE_STATEMENT)
  const isCreatingSample = computed(() => busySection.value === problemBusySection.CREATE_SAMPLE)
  const isDeletingLastSample = computed(() => busySection.value === problemBusySection.DELETE_LAST_SAMPLE)
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
  const selectedProblemId = computed(() => Number(selectedProblemDetail.value?.problem_id ?? 0))
  const clearActionFeedback = () => problemActionFeedback.setActionFeedback({
    message: '',
    error: ''
  })

  function withSelectedProblemGuard(canRun){
    return Boolean(authState.token) &&
      Boolean(selectedProblemDetail.value) &&
      Boolean(canRun)
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

    if (!withSelectedProblemGuard(canRun) || !problemDetail) {
      return
    }

    const problemId = problemDetail.problem_id

    return runBusyAction({
      busySection,
      section,
      clearFeedback: clearActionFeedback,
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

        problemActionFeedback.setActionFeedback({
          message: successMessage(problemId, response)
        })
      },
      onError: (error) => {
        problemActionFeedback.setActionFeedback({
          error: formatApiError(error, {
            fallback: fallbackError
          })
        })
      }
    })
  }

  async function handleSaveTitle(){
    const nextTitle = editorDraft.titleDraft.value.trim()

    return runProblemMutation({
      section: problemBusySection.SAVE_TITLE,
      canRun: editorDraft.canSaveTitle.value,
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
        editorDraft.titleDraft.value = nextTitle
      },
      successMessage(problemId){
        return `문제 #${formatCount(problemId)} 제목을 저장했습니다.`
      },
      fallbackError: '문제 제목을 저장하지 못했습니다.'
    })
  }

  async function handleSaveLimits(){
    const nextTimeLimit = parsePositiveInteger(editorDraft.timeLimitDraft.value)
    const nextMemoryLimit = parsePositiveInteger(editorDraft.memoryLimitDraft.value)

    if (nextTimeLimit == null || nextMemoryLimit == null) {
      return
    }

    return runProblemMutation({
      section: problemBusySection.SAVE_LIMITS,
      canRun: editorDraft.canSaveLimits.value,
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
        editorDraft.timeLimitDraft.value = String(nextTimeLimit)
        editorDraft.memoryLimitDraft.value = String(nextMemoryLimit)
      },
      successMessage(problemId){
        return `문제 #${formatCount(problemId)} 제한을 저장했습니다.`
      },
      fallbackError: '문제 제한을 저장하지 못했습니다.'
    })
  }

  async function handleSaveStatement(){
    const nextStatement = {
      description: editorDraft.descriptionDraft.value,
      input_format: editorDraft.inputFormatDraft.value,
      output_format: editorDraft.outputFormatDraft.value,
      note: editorDraft.noteDraft.value === '' ? null : editorDraft.noteDraft.value
    }

    return runProblemMutation({
      section: problemBusySection.SAVE_STATEMENT,
      canRun: editorDraft.canSaveStatement.value,
      runRequest(problemId){
        return updateProblemStatement(problemId, nextStatement, authState.token)
      },
      patchDetail(problemDetail){
        return problemDetail
          ? {
            ...problemDetail,
            statement: {
              description: editorDraft.descriptionDraft.value,
              input_format: editorDraft.inputFormatDraft.value,
              output_format: editorDraft.outputFormatDraft.value,
              note: editorDraft.noteDraft.value
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
      clearFeedback: clearActionFeedback,
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
        editorDraft.syncSampleDrafts(nextSamples)
        problemDetailResource.applyProblemVersion(problemId, response.version)
        problemActionFeedback.setActionFeedback({
          message: `예제 ${formatCount(nextSampleOrder)}를 추가했습니다.`
        })
      },
      onError: (error) => {
        problemActionFeedback.setActionFeedback({
          error: formatApiError(error, {
            fallback: '공개 예제를 추가하지 못했습니다.'
          })
        })
      }
    })
  }

  async function handleSaveSample(sampleOrder){
    if (!authState.token || !selectedProblemDetail.value || !editorDraft.canSaveSample(sampleOrder)) {
      return
    }

    const problemId = selectedProblemDetail.value.problem_id
    const sampleDraft = editorDraft.getSampleDraft(sampleOrder)
    if (!sampleDraft) {
      return
    }

    return runBusyAction({
      busySection,
      section: makeProblemSampleBusySection(sampleOrder),
      clearFeedback: clearActionFeedback,
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
        editorDraft.syncSampleDrafts(nextSamples)
        problemDetailResource.applyProblemVersion(problemId, response.version)
        problemActionFeedback.setActionFeedback({
          message: `예제 ${formatCount(sampleOrder)}를 저장했습니다.`
        })
      },
      onError: (error) => {
        problemActionFeedback.setActionFeedback({
          error: formatApiError(error, {
            fallback: '공개 예제를 저장하지 못했습니다.'
          })
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
      clearFeedback: clearActionFeedback,
      run: async () => {
        const response = await deleteProblemSample(problemId, authState.token)
        const nextSamples = selectedProblemDetail.value.samples.filter(
          (sample) => sample.sample_order !== lastSample.sample_order
        )

        problemDetailResource.setSelectedProblemSamples(nextSamples)
        editorDraft.syncSampleDrafts(nextSamples)
        problemDetailResource.applyProblemVersion(problemId, response.version)
        problemActionFeedback.setActionFeedback({
          message: `예제 ${formatCount(lastSample.sample_order)}를 삭제했습니다.`
        })
      },
      onError: (error) => {
        problemActionFeedback.setActionFeedback({
          error: formatApiError(error, {
            fallback: '공개 예제를 삭제하지 못했습니다.'
          })
        })
      }
    })
  }

  const testcaseZipUploadAction = useTestcaseZipUploadAction({
    authState,
    busySection,
    uploadSection: problemBusySection.UPLOAD_TESTCASE_ZIP,
    selectedProblemId,
    testcaseZipFile: editorDraft.testcaseZipFile,
    clearFeedback: clearActionFeedback,
    resetTestcaseZipSelection: editorDraft.resetTestcaseZipSelection,
    async afterUpload(response, problemId){
      problemDetailResource.applyProblemVersion(problemId, response.version)
      await loadSelectedProblem(problemId, {
        skipTestcaseZipReset: true
      })
    },
    showSuccess(message){
      problemActionFeedback.setActionFeedback({
        message
      })
    },
    showError(error){
      problemActionFeedback.setActionFeedback({
        error
      })
    },
    formatSuccessMessage(response){
      const testcaseCount = Number(response.testcase_count ?? 0)
      return `비공개 테스트케이스 ${formatCount(testcaseCount)}개를 업로드했습니다.`
    },
    fallbackError: '비공개 테스트케이스 ZIP을 업로드하지 못했습니다.'
  })

  return {
    isSavingTitle,
    isSavingLimits,
    isSavingStatement,
    isCreatingSample,
    isDeletingLastSample,
    isUploadingTestcaseZip: testcaseZipUploadAction.isUploadingTestcaseZip,
    canCreateSample,
    canUploadTestcaseZip: testcaseZipUploadAction.canUploadTestcaseZip,
    canDeleteLastSample,
    handleSaveTitle,
    handleSaveLimits,
    handleSaveStatement,
    handleCreateSample,
    handleSaveSample,
    handleDeleteLastSample,
    handleUploadTestcaseZip: testcaseZipUploadAction.handleUploadTestcaseZip
  }
}

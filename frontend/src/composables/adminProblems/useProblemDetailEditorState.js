import { computed, ref } from 'vue'

import { runBusyAction } from '@/composables/adminShared/runBusyAction'
import { problemBusySection } from '@/composables/adminProblems/problemBusySection'
import {
  updateProblemLimits,
  updateProblemStatement,
  updateProblemTitle
} from '@/api/problemAdminApi'
import { formatApiError } from '@/utils/apiError'
import { parsePositiveInteger } from '@/utils/parse'

export function useProblemDetailEditorState({
  authState,
  busySection,
  formatCount,
  selectedProblemDetail,
  problemDetailResource,
  problemCatalogResource,
  problemActionFeedback
}){
  const titleDraft = ref('')
  const timeLimitDraft = ref('')
  const memoryLimitDraft = ref('')
  const descriptionDraft = ref('')
  const inputFormatDraft = ref('')
  const outputFormatDraft = ref('')
  const noteDraft = ref('')
  const isSavingTitle = computed(() => busySection.value === problemBusySection.SAVE_TITLE)
  const isSavingLimits = computed(() => busySection.value === problemBusySection.SAVE_LIMITS)
  const isSavingStatement = computed(() => busySection.value === problemBusySection.SAVE_STATEMENT)
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
  const clearActionFeedback = () => problemActionFeedback.setActionFeedback({
    message: '',
    error: ''
  })

  function reset(){
    titleDraft.value = ''
    timeLimitDraft.value = ''
    memoryLimitDraft.value = ''
    descriptionDraft.value = ''
    inputFormatDraft.value = ''
    outputFormatDraft.value = ''
    noteDraft.value = ''
  }

  function assignFromProblemDetail(problemDetail){
    titleDraft.value = problemDetail.title
    timeLimitDraft.value = String(problemDetail.limits.time_limit_ms || '')
    memoryLimitDraft.value = String(problemDetail.limits.memory_limit_mb || '')
    descriptionDraft.value = problemDetail.statement.description
    inputFormatDraft.value = problemDetail.statement.input_format
    outputFormatDraft.value = problemDetail.statement.output_format
    noteDraft.value = problemDetail.statement.note
  }

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

  return {
    titleDraft,
    timeLimitDraft,
    memoryLimitDraft,
    descriptionDraft,
    inputFormatDraft,
    outputFormatDraft,
    noteDraft,
    canSaveTitle,
    canSaveLimits,
    canSaveStatement,
    isSavingTitle,
    isSavingLimits,
    isSavingStatement,
    reset,
    assignFromProblemDetail,
    handleSaveTitle,
    handleSaveLimits,
    handleSaveStatement
  }
}

import { computed } from 'vue'

import { runBusyAction } from '@/composables/adminShared/runBusyAction'
import { problemBusySection } from '@/composables/adminProblems/problemBusySection'
import {
  updateProblemLimits,
  updateProblemStatement,
  updateProblemTitle
} from '@/api/problemAdminApi'
import { formatApiError } from '@/utils/apiError'
import { parsePositiveInteger } from '@/utils/parse'

export function useProblemMutationActions({
  authState,
  busySection,
  formatCount,
  draft,
  feedback,
  problemDetailResource,
  problemCatalogResource
}){
  const isSavingTitle = computed(() => busySection.value === problemBusySection.SAVE_TITLE)
  const isSavingLimits = computed(() => busySection.value === problemBusySection.SAVE_LIMITS)
  const isSavingStatement = computed(() => busySection.value === problemBusySection.SAVE_STATEMENT)
  const clearActionFeedback = () => feedback.setActionFeedback({
    message: '',
    error: ''
  })

  function withSelectedProblemGuard(canRun){
    return Boolean(authState.token) &&
      Boolean(problemDetailResource.selectedProblemDetail.value) &&
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
    const selectedProblemDetail = problemDetailResource.selectedProblemDetail.value

    if (!withSelectedProblemGuard(canRun)) {
      return
    }

    const problemId = selectedProblemDetail.problem_id

    return runBusyAction({
      busySection,
      section,
      clearFeedback: clearActionFeedback,
      run: async () => {
        const response = await runRequest(problemId)

        if (typeof patchDetail === 'function') {
          problemDetailResource.setSelectedProblemDetail((problemDetail) => patchDetail(problemDetail, response))
        }

        if (typeof afterSuccess === 'function') {
          afterSuccess(problemId, response)
        }

        problemDetailResource.applyProblemVersion(problemId, response.version)

        if (patchSummary) {
          problemCatalogResource.mergeProblemSummary(problemId, patchSummary)
        }

        feedback.setActionFeedback({
          message: successMessage(problemId, response)
        })
      },
      onError: (error) => {
        feedback.setActionFeedback({
          error: formatApiError(error, {
            fallback: fallbackError
          })
        })
      }
    })
  }

  async function handleSaveTitle(){
    const nextTitle = draft.titleDraft.value.trim()

    return runProblemMutation({
      section: problemBusySection.SAVE_TITLE,
      canRun: draft.canSaveTitle.value,
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
        draft.titleDraft.value = nextTitle
      },
      successMessage(problemId){
        return `문제 #${formatCount(problemId)} 제목을 저장했습니다.`
      },
      fallbackError: '문제 제목을 저장하지 못했습니다.'
    })
  }

  async function handleSaveLimits(){
    const nextTimeLimit = parsePositiveInteger(draft.timeLimitDraft.value)
    const nextMemoryLimit = parsePositiveInteger(draft.memoryLimitDraft.value)

    if (nextTimeLimit == null || nextMemoryLimit == null) {
      return
    }

    return runProblemMutation({
      section: problemBusySection.SAVE_LIMITS,
      canRun: draft.canSaveLimits.value,
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
        draft.timeLimitDraft.value = String(nextTimeLimit)
        draft.memoryLimitDraft.value = String(nextMemoryLimit)
      },
      successMessage(problemId){
        return `문제 #${formatCount(problemId)} 제한을 저장했습니다.`
      },
      fallbackError: '문제 제한을 저장하지 못했습니다.'
    })
  }

  async function handleSaveStatement(){
    const nextStatement = {
      description: draft.descriptionDraft.value,
      input_format: draft.inputFormatDraft.value,
      output_format: draft.outputFormatDraft.value,
      note: draft.noteDraft.value === '' ? null : draft.noteDraft.value
    }

    return runProblemMutation({
      section: problemBusySection.SAVE_STATEMENT,
      canRun: draft.canSaveStatement.value,
      runRequest(problemId){
        return updateProblemStatement(problemId, nextStatement, authState.token)
      },
      patchDetail(problemDetail){
        return problemDetail
          ? {
            ...problemDetail,
            statement: {
              description: draft.descriptionDraft.value,
              input_format: draft.inputFormatDraft.value,
              output_format: draft.outputFormatDraft.value,
              note: draft.noteDraft.value
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
    isSavingTitle,
    isSavingLimits,
    isSavingStatement,
    handleSaveTitle,
    handleSaveLimits,
    handleSaveStatement
  }
}

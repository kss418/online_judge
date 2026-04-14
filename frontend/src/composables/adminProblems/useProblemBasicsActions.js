import { computed } from 'vue'

import { runBusyAction } from '@/composables/adminShared/runBusyAction'
import { problemBusySection } from '@/composables/adminProblems/problemBusySection'
import {
  updateProblemLimits,
  updateProblemTitle
} from '@/api/problemAdminApi'
import { formatApiError } from '@/utils/apiError'
import { parsePositiveInteger } from '@/utils/parse'

export function useProblemBasicsActions({
  authState,
  busySection,
  formatCount,
  selectedProblemDetail,
  titleDraft,
  timeLimitDraft,
  memoryLimitDraft,
  canSaveTitle,
  canSaveLimits,
  applySelectedProblemVersion,
  mergeProblemSummary,
  setActionFeedback
}){
  const isSavingTitle = computed(() => busySection.value === problemBusySection.SAVE_TITLE)
  const isSavingLimits = computed(() => busySection.value === problemBusySection.SAVE_LIMITS)
  const clearActionFeedback = () => setActionFeedback({
    message: '',
    error: ''
  })

  async function handleSaveTitle(){
    if (!authState.token || !selectedProblemDetail.value || !canSaveTitle.value) {
      return
    }

    const problemId = selectedProblemDetail.value.problem_id
    const nextTitle = titleDraft.value.trim()

    return runBusyAction({
      busySection,
      section: problemBusySection.SAVE_TITLE,
      clearFeedback: clearActionFeedback,
      run: async () => {
        const response = await updateProblemTitle(problemId, {
          title: nextTitle
        }, authState.token)

        selectedProblemDetail.value = {
          ...selectedProblemDetail.value,
          title: nextTitle
        }
        titleDraft.value = nextTitle
        applySelectedProblemVersion(problemId, response.version)
        mergeProblemSummary(problemId, {
          title: nextTitle
        })
        setActionFeedback({
          message: `문제 #${formatCount(problemId)} 제목을 저장했습니다.`
        })
      },
      onError: (error) => {
        setActionFeedback({
          error: formatApiError(error, {
            fallback: '문제 제목을 저장하지 못했습니다.'
          })
        })
      }
    })
  }

  async function handleSaveLimits(){
    if (!authState.token || !selectedProblemDetail.value || !canSaveLimits.value) {
      return
    }

    const nextTimeLimit = parsePositiveInteger(timeLimitDraft.value)
    const nextMemoryLimit = parsePositiveInteger(memoryLimitDraft.value)
    if (nextTimeLimit == null || nextMemoryLimit == null) {
      return
    }

    const problemId = selectedProblemDetail.value.problem_id

    return runBusyAction({
      busySection,
      section: problemBusySection.SAVE_LIMITS,
      clearFeedback: clearActionFeedback,
      run: async () => {
        const response = await updateProblemLimits(problemId, {
          time_limit_ms: nextTimeLimit,
          memory_limit_mb: nextMemoryLimit
        }, authState.token)

        selectedProblemDetail.value = {
          ...selectedProblemDetail.value,
          limits: {
            time_limit_ms: nextTimeLimit,
            memory_limit_mb: nextMemoryLimit
          }
        }
        timeLimitDraft.value = String(nextTimeLimit)
        memoryLimitDraft.value = String(nextMemoryLimit)
        applySelectedProblemVersion(problemId, response.version)
        mergeProblemSummary(problemId, {
          time_limit_ms: nextTimeLimit,
          memory_limit_mb: nextMemoryLimit
        })
        setActionFeedback({
          message: `문제 #${formatCount(problemId)} 제한을 저장했습니다.`
        })
      },
      onError: (error) => {
        setActionFeedback({
          error: formatApiError(error, {
            fallback: '문제 제한을 저장하지 못했습니다.'
          })
        })
      }
    })
  }

  return {
    isSavingTitle,
    isSavingLimits,
    handleSaveTitle,
    handleSaveLimits
  }
}

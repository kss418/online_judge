import { computed } from 'vue'

import {
  updateProblemLimits,
  updateProblemTitle
} from '@/api/problemAdminApi'
import { parsePositiveInteger } from '@/composables/adminProblems/problemHelpers'
import { formatApiError } from '@/utils/apiError'

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
  const isSavingTitle = computed(() => busySection.value === 'title')
  const isSavingLimits = computed(() => busySection.value === 'limits')

  async function handleSaveTitle(){
    if (!authState.token || !selectedProblemDetail.value || !canSaveTitle.value) {
      return
    }

    const problemId = selectedProblemDetail.value.problem_id
    busySection.value = 'title'
    setActionFeedback({
      message: '',
      error: ''
    })

    try {
      const nextTitle = titleDraft.value.trim()
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
    } catch (error) {
      setActionFeedback({
        error: formatApiError(error, {
          fallback: '문제 제목을 저장하지 못했습니다.'
        })
      })
    } finally {
      busySection.value = ''
    }
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
    busySection.value = 'limits'
    setActionFeedback({
      message: '',
      error: ''
    })

    try {
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
    } catch (error) {
      setActionFeedback({
        error: formatApiError(error, {
          fallback: '문제 제한을 저장하지 못했습니다.'
        })
      })
    } finally {
      busySection.value = ''
    }
  }

  return {
    isSavingTitle,
    isSavingLimits,
    handleSaveTitle,
    handleSaveLimits
  }
}

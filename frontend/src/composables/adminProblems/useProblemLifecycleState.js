import { computed, ref } from 'vue'

import { runBusyAction } from '@/composables/adminShared/runBusyAction'
import { problemBusySection } from '@/composables/adminProblems/problemBusySection'
import {
  createProblem,
  deleteProblem,
  rejudgeProblem
} from '@/api/problemAdminApi'
import { formatApiError } from '@/utils/apiError'

export function useProblemLifecycleState({
  authState,
  busySection,
  formatCount,
  problemActionFeedback,
  selectedProblemDetail,
  loadProblems,
  onCreatedProblem
}){
  const newProblemTitle = ref('')
  const canCreateProblem = computed(() =>
    Boolean(authState.token) &&
    !busySection.value &&
    Boolean(newProblemTitle.value.trim())
  )
  const isCreatingProblem = computed(() => busySection.value === problemBusySection.CREATE)
  const isRejudgingProblem = computed(() => busySection.value === problemBusySection.REJUDGE)
  const isDeletingProblem = computed(() => busySection.value === problemBusySection.DELETE)
  const clearActionFeedback = () => problemActionFeedback.setActionFeedback({
    message: '',
    error: ''
  })

  function updateNewProblemTitle(value){
    newProblemTitle.value = value
  }

  async function handleCreateProblem(){
    if (!authState.token || !canCreateProblem.value) {
      return
    }

    return runBusyAction({
      busySection,
      section: problemBusySection.CREATE,
      clearFeedback: clearActionFeedback,
      run: async () => {
        const response = await createProblem({
          title: newProblemTitle.value.trim()
        }, authState.token)
        const createdProblemId = Number(response.problem_id ?? 0)

        newProblemTitle.value = ''
        problemActionFeedback.setActionFeedback({
          message: `문제 #${formatCount(createdProblemId)}를 생성했습니다.`
        })

        if (typeof onCreatedProblem === 'function') {
          await onCreatedProblem(createdProblemId)
        }
      },
      onError: (error) => {
        problemActionFeedback.setActionFeedback({
          error: formatApiError(error, {
            fallback: '문제를 생성하지 못했습니다.'
          })
        })
      }
    })
  }

  async function handleRejudgeProblem(){
    if (!authState.token || !selectedProblemDetail.value || !problemActionFeedback.canRejudgeSelectedProblem.value) {
      return
    }

    const rejudgingProblemId = selectedProblemDetail.value.problem_id

    return runBusyAction({
      busySection,
      section: problemBusySection.REJUDGE,
      clearFeedback: clearActionFeedback,
      run: async () => {
        await rejudgeProblem(rejudgingProblemId, authState.token)
        problemActionFeedback.closeRejudgeDialog(true)
        problemActionFeedback.setActionFeedback({
          message: `문제 #${formatCount(rejudgingProblemId)} 재채점을 요청했습니다.`
        })
      },
      onError: (error) => {
        problemActionFeedback.setActionFeedback({
          error: formatApiError(error, {
            fallback: '문제 재채점을 요청하지 못했습니다.'
          })
        })
      }
    })
  }

  async function handleDeleteProblem(){
    if (!authState.token || !selectedProblemDetail.value || !problemActionFeedback.canDeleteSelectedProblem.value) {
      return
    }

    const deletingProblemId = selectedProblemDetail.value.problem_id

    return runBusyAction({
      busySection,
      section: problemBusySection.DELETE,
      clearFeedback: clearActionFeedback,
      run: async () => {
        await deleteProblem(deletingProblemId, authState.token)
        problemActionFeedback.closeDeleteDialog(true)
        problemActionFeedback.setActionFeedback({
          message: `문제 #${formatCount(deletingProblemId)}를 삭제했습니다.`
        })
        await loadProblems()
      },
      onError: (error) => {
        problemActionFeedback.setActionFeedback({
          error: formatApiError(error, {
            fallback: '문제를 삭제하지 못했습니다.'
          })
        })
      }
    })
  }

  return {
    newProblemTitle,
    canCreateProblem,
    isCreatingProblem,
    isRejudgingProblem,
    isDeletingProblem,
    updateNewProblemTitle,
    handleCreateProblem,
    handleRejudgeProblem,
    handleDeleteProblem
  }
}

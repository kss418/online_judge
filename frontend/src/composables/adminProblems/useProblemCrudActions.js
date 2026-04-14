import { computed } from 'vue'

import { problemBusySection } from '@/composables/adminProblems/problemBusySection'
import {
  createProblem,
  deleteProblem,
  rejudgeProblem
} from '@/api/problemAdminApi'
import { formatApiError } from '@/utils/apiError'

export function useProblemCrudActions({
  authState,
  busySection,
  formatCount,
  newProblemTitle,
  selectedProblemDetail,
  canDeleteSelectedProblem,
  canRejudgeSelectedProblem,
  setActionFeedback,
  closeDeleteDialog,
  closeRejudgeDialog,
  onCreatedProblem,
  loadProblems
}){
  const canCreateProblem = computed(() =>
    Boolean(authState.token) &&
    !busySection.value &&
    Boolean(newProblemTitle.value.trim())
  )
  const isCreatingProblem = computed(() => busySection.value === problemBusySection.CREATE)
  const isRejudgingProblem = computed(() => busySection.value === problemBusySection.REJUDGE)
  const isDeletingProblem = computed(() => busySection.value === problemBusySection.DELETE)

  async function handleCreateProblem(){
    if (!authState.token || !canCreateProblem.value) {
      return
    }

    busySection.value = problemBusySection.CREATE
    setActionFeedback({
      message: '',
      error: ''
    })

    try {
      const response = await createProblem({
        title: newProblemTitle.value.trim()
      }, authState.token)
      const createdProblemId = Number(response.problem_id ?? 0)

      newProblemTitle.value = ''
      setActionFeedback({
        message: `문제 #${formatCount(createdProblemId)}를 생성했습니다.`
      })

      if (typeof onCreatedProblem === 'function') {
        await onCreatedProblem(createdProblemId)
      }
    } catch (error) {
      setActionFeedback({
        error: formatApiError(error, {
          fallback: '문제를 생성하지 못했습니다.'
        })
      })
    } finally {
      busySection.value = ''
    }
  }

  async function handleRejudgeProblem(){
    if (!authState.token || !selectedProblemDetail.value || !canRejudgeSelectedProblem.value) {
      return
    }

    const rejudgingProblemId = selectedProblemDetail.value.problem_id
    busySection.value = problemBusySection.REJUDGE
    setActionFeedback({
      message: '',
      error: ''
    })

    try {
      await rejudgeProblem(rejudgingProblemId, authState.token)
      closeRejudgeDialog(true)
      setActionFeedback({
        message: `문제 #${formatCount(rejudgingProblemId)} 재채점을 요청했습니다.`
      })
    } catch (error) {
      setActionFeedback({
        error: formatApiError(error, {
          fallback: '문제 재채점을 요청하지 못했습니다.'
        })
      })
    } finally {
      busySection.value = ''
    }
  }

  async function handleDeleteProblem(){
    if (!authState.token || !selectedProblemDetail.value || !canDeleteSelectedProblem.value) {
      return
    }

    const deletingProblemId = selectedProblemDetail.value.problem_id
    busySection.value = problemBusySection.DELETE
    setActionFeedback({
      message: '',
      error: ''
    })

    try {
      await deleteProblem(deletingProblemId, authState.token)
      closeDeleteDialog(true)
      setActionFeedback({
        message: `문제 #${formatCount(deletingProblemId)}를 삭제했습니다.`
      })
      await loadProblems()
    } catch (error) {
      setActionFeedback({
        error: formatApiError(error, {
          fallback: '문제를 삭제하지 못했습니다.'
        })
      })
    } finally {
      busySection.value = ''
    }
  }

  return {
    canCreateProblem,
    isCreatingProblem,
    isRejudgingProblem,
    isDeletingProblem,
    handleCreateProblem,
    handleRejudgeProblem,
    handleDeleteProblem
  }
}

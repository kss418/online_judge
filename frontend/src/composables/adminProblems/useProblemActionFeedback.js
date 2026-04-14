import { computed, ref } from 'vue'

export function useProblemActionFeedback({
  selectedProblemDetail,
  busySection
}){
  const actionMessage = ref('')
  const actionErrorMessage = ref('')
  const rejudgeDialogOpen = ref(false)
  const rejudgeConfirmProblemId = ref('')
  const rejudgeConfirmTitle = ref('')
  const deleteDialogOpen = ref(false)
  const deleteConfirmProblemId = ref('')
  const deleteConfirmTitle = ref('')

  const canDeleteSelectedProblem = computed(() => {
    if (!selectedProblemDetail.value || !deleteDialogOpen.value || busySection.value) {
      return false
    }

    const expectedProblemId = String(selectedProblemDetail.value.problem_id)
    return (
      deleteConfirmProblemId.value.trim() === expectedProblemId &&
      deleteConfirmTitle.value === selectedProblemDetail.value.title
    )
  })

  const canRejudgeSelectedProblem = computed(() => {
    if (!selectedProblemDetail.value || !rejudgeDialogOpen.value || busySection.value) {
      return false
    }

    const expectedProblemId = String(selectedProblemDetail.value.problem_id)
    return (
      rejudgeConfirmProblemId.value.trim() === expectedProblemId &&
      rejudgeConfirmTitle.value === selectedProblemDetail.value.title
    )
  })

  function clearActionError(){
    actionErrorMessage.value = ''
  }

  function setActionFeedback({ message, error }){
    if (typeof message !== 'undefined') {
      actionMessage.value = message
    }

    if (typeof error !== 'undefined') {
      actionErrorMessage.value = error
    }
  }

  function openDeleteDialog(){
    if (!selectedProblemDetail.value || busySection.value) {
      return
    }

    deleteConfirmProblemId.value = ''
    deleteConfirmTitle.value = ''
    deleteDialogOpen.value = true
  }

  function openRejudgeDialog(){
    if (!selectedProblemDetail.value || busySection.value) {
      return
    }

    rejudgeConfirmProblemId.value = ''
    rejudgeConfirmTitle.value = ''
    rejudgeDialogOpen.value = true
  }

  function closeDeleteDialog(force = false){
    if (!force && busySection.value === 'delete') {
      return
    }

    deleteDialogOpen.value = false
    deleteConfirmProblemId.value = ''
    deleteConfirmTitle.value = ''
  }

  function closeRejudgeDialog(force = false){
    if (!force && busySection.value === 'rejudge') {
      return
    }

    rejudgeDialogOpen.value = false
    rejudgeConfirmProblemId.value = ''
    rejudgeConfirmTitle.value = ''
  }

  function resetActionState(){
    actionMessage.value = ''
    actionErrorMessage.value = ''
    closeRejudgeDialog(true)
    closeDeleteDialog(true)
  }

  return {
    actionMessage,
    actionErrorMessage,
    rejudgeDialogOpen,
    rejudgeConfirmProblemId,
    rejudgeConfirmTitle,
    deleteDialogOpen,
    deleteConfirmProblemId,
    deleteConfirmTitle,
    canDeleteSelectedProblem,
    canRejudgeSelectedProblem,
    clearActionError,
    setActionFeedback,
    openDeleteDialog,
    openRejudgeDialog,
    closeDeleteDialog,
    closeRejudgeDialog,
    resetActionState
  }
}

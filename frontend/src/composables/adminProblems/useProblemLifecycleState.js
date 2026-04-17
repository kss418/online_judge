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
  selectedProblemDetail,
  loadProblems,
  onCreatedProblem,
  showSuccessNotice,
  showErrorNotice
}){
  const newProblemTitle = ref('')
  const rejudgeDialogOpen = ref(false)
  const rejudgeConfirmProblemId = ref('')
  const rejudgeConfirmTitle = ref('')
  const deleteDialogOpen = ref(false)
  const deleteConfirmProblemId = ref('')
  const deleteConfirmTitle = ref('')
  const canCreateProblem = computed(() =>
    Boolean(authState.token) &&
    !busySection.value &&
    Boolean(newProblemTitle.value.trim())
  )
  const isCreatingProblem = computed(() => busySection.value === problemBusySection.CREATE)
  const isRejudgingProblem = computed(() => busySection.value === problemBusySection.REJUDGE)
  const isDeletingProblem = computed(() => busySection.value === problemBusySection.DELETE)
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

  function updateNewProblemTitle(value){
    newProblemTitle.value = value
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
    if (!force && busySection.value === problemBusySection.DELETE) {
      return
    }

    deleteDialogOpen.value = false
    deleteConfirmProblemId.value = ''
    deleteConfirmTitle.value = ''
  }

  function closeRejudgeDialog(force = false){
    if (!force && busySection.value === problemBusySection.REJUDGE) {
      return
    }

    rejudgeDialogOpen.value = false
    rejudgeConfirmProblemId.value = ''
    rejudgeConfirmTitle.value = ''
  }

  function resetActionState(){
    newProblemTitle.value = ''
    closeRejudgeDialog(true)
    closeDeleteDialog(true)
  }

  async function handleCreateProblem(){
    if (!authState.token || !canCreateProblem.value) {
      return
    }

    return runBusyAction({
      busySection,
      section: problemBusySection.CREATE,
      run: async () => {
        const response = await createProblem({
          title: newProblemTitle.value.trim()
        }, authState.token)
        const createdProblemId = Number(response.problem_id ?? 0)

        newProblemTitle.value = ''

        if (typeof onCreatedProblem === 'function') {
          await onCreatedProblem(createdProblemId)
        }

        showSuccessNotice(`문제 #${formatCount(createdProblemId)}를 생성했습니다.`)
      },
      onError: (error) => {
        showErrorNotice(formatApiError(error, {
          fallback: '문제를 생성하지 못했습니다.'
        }), {
          duration: 5000
        })
      }
    })
  }

  async function handleRejudgeProblem(){
    if (!authState.token || !selectedProblemDetail.value || !canRejudgeSelectedProblem.value) {
      return
    }

    const rejudgingProblemId = selectedProblemDetail.value.problem_id

    return runBusyAction({
      busySection,
      section: problemBusySection.REJUDGE,
      run: async () => {
        await rejudgeProblem(rejudgingProblemId, authState.token)
        closeRejudgeDialog(true)
        showSuccessNotice(`문제 #${formatCount(rejudgingProblemId)} 재채점을 요청했습니다.`)
      },
      onError: (error) => {
        showErrorNotice(formatApiError(error, {
          fallback: '문제 재채점을 요청하지 못했습니다.'
        }), {
          duration: 5000
        })
      }
    })
  }

  async function handleDeleteProblem(){
    if (!authState.token || !selectedProblemDetail.value || !canDeleteSelectedProblem.value) {
      return
    }

    const deletingProblemId = selectedProblemDetail.value.problem_id

    return runBusyAction({
      busySection,
      section: problemBusySection.DELETE,
      run: async () => {
        await deleteProblem(deletingProblemId, authState.token)
        closeDeleteDialog(true)
        await loadProblems()
        showSuccessNotice(`문제 #${formatCount(deletingProblemId)}를 삭제했습니다.`)
      },
      onError: (error) => {
        showErrorNotice(formatApiError(error, {
          fallback: '문제를 삭제하지 못했습니다.'
        }), {
          duration: 5000
        })
      }
    })
  }

  return {
    newProblemTitle,
    rejudgeDialogOpen,
    rejudgeConfirmProblemId,
    rejudgeConfirmTitle,
    deleteDialogOpen,
    deleteConfirmProblemId,
    deleteConfirmTitle,
    canCreateProblem,
    canDeleteSelectedProblem,
    canRejudgeSelectedProblem,
    isCreatingProblem,
    isRejudgingProblem,
    isDeletingProblem,
    updateNewProblemTitle,
    openDeleteDialog,
    openRejudgeDialog,
    closeDeleteDialog,
    closeRejudgeDialog,
    resetActionState,
    handleCreateProblem,
    handleRejudgeProblem,
    handleDeleteProblem
  }
}

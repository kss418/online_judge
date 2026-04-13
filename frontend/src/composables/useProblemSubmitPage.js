import { computed, onMounted, watch } from 'vue'
import { useRoute } from 'vue-router'

import { authStore } from '@/stores/auth/authStore'
import { useProblemSubmitAction } from '@/composables/problemSubmit/useProblemSubmitAction'
import { useProblemSubmitEditor } from '@/composables/problemSubmit/useProblemSubmitEditor'
import { useProblemSubmitResource } from '@/composables/problemSubmit/useProblemSubmitResource'

export function useProblemSubmitPage(){
  const route = useRoute()
  const {
    state: authState,
    isAuthenticated,
    initializeAuth
  } = authStore
  const numericProblemId = computed(() => Number.parseInt(route.params.problemId, 10))

  let problemSubmitAction

  const problemSubmitResource = useProblemSubmitResource({
    authState,
    isAuthenticated,
    numericProblemId
  })
  const problemSubmitEditor = useProblemSubmitEditor({
    supportedLanguages: problemSubmitResource.supportedLanguages,
    isSubmittingSubmission: computed(() => problemSubmitAction?.isSubmittingSubmission.value ?? false),
    submitSolution: () => problemSubmitAction?.submitSolution()
  })

  problemSubmitAction = useProblemSubmitAction({
    authState,
    isAuthenticated,
    numericProblemId,
    selectedLanguage: problemSubmitEditor.selectedLanguage,
    sourceCode: problemSubmitEditor.sourceCode,
    isLoadingLanguages: problemSubmitResource.isLoadingLanguages,
    isSubmissionBanActive: problemSubmitResource.isSubmissionBanActive,
    refreshSubmissionBan: problemSubmitResource.loadMySubmissionBan
  })

  watch(numericProblemId, () => {
    problemSubmitAction.resetSubmitFeedback()
    problemSubmitEditor.resetSourceCode()
    void problemSubmitResource.loadProblemDetail()
  })

  watch(
    [
      () => authState.initialized,
      () => authState.token,
      isAuthenticated
    ],
    ([initialized, token, authenticated]) => {
      if (!initialized || !authenticated || !token) {
        problemSubmitResource.resetSubmissionBan()
        return
      }

      void problemSubmitResource.loadMySubmissionBan()
    },
    {
      immediate: true
    }
  )

  onMounted(() => {
    void initializeAuth()
    void problemSubmitResource.loadProblemDetail()
    void problemSubmitResource.loadSupportedLanguageList()
    problemSubmitEditor.scheduleEditorResize()
  })

  return {
    authState,
    isAuthenticated,
    numericProblemId,
    isLoadingProblem: problemSubmitResource.isLoadingProblem,
    problemErrorMessage: problemSubmitResource.problemErrorMessage,
    problemDetail: problemSubmitResource.problemDetail,
    isLoadingLanguages: problemSubmitResource.isLoadingLanguages,
    languageErrorMessage: problemSubmitResource.languageErrorMessage,
    supportedLanguages: problemSubmitResource.supportedLanguages,
    isSubmissionBanActive: problemSubmitResource.isSubmissionBanActive,
    submissionBanUntilText: problemSubmitResource.submissionBanUntilText,
    submissionBanRemainingText: problemSubmitResource.submissionBanRemainingText,
    selectedLanguage: problemSubmitEditor.selectedLanguage,
    sourceCode: problemSubmitEditor.sourceCode,
    sourceEditorElement: problemSubmitEditor.sourceEditorElement,
    activeLanguage: problemSubmitEditor.activeLanguage,
    editorLineNumbers: problemSubmitEditor.editorLineNumbers,
    editorPlaceholder: problemSubmitEditor.editorPlaceholder,
    isSubmittingSubmission: problemSubmitAction.isSubmittingSubmission,
    submitErrorMessage: problemSubmitAction.submitErrorMessage,
    submissionSuccessMessage: problemSubmitAction.submissionSuccessMessage,
    canSubmit: problemSubmitAction.canSubmit,
    handleSourceEditorInput: problemSubmitEditor.handleSourceEditorInput,
    handleEditorKeydown: problemSubmitEditor.handleEditorKeydown,
    submitSolution: problemSubmitAction.submitSolution
  }
}

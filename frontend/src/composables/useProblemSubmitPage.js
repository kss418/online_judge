import { computed, onMounted, ref, watch } from 'vue'
import { useRoute } from 'vue-router'

import { authStore } from '@/stores/auth/authStore'
import { useProblemSubmitAction } from '@/composables/problemSubmit/useProblemSubmitAction'
import { useProblemSubmitEditor } from '@/composables/problemSubmit/useProblemSubmitEditor'
import { useProblemSubmitResource } from '@/composables/problemSubmit/useProblemSubmitResource'
import { parsePositiveInteger } from '@/utils/parse'

export function useProblemSubmitPage(){
  const route = useRoute()
  const {
    state: authState,
    isAuthenticated,
    initializeAuth
  } = authStore
  const numericProblemId = computed(() => parsePositiveInteger(route.params.problemId) ?? 0)
  const selectedLanguage = ref('')
  const sourceCode = ref('')
  const sourceEditorElement = ref(null)

  const problemSubmitResource = useProblemSubmitResource({
    authState,
    isAuthenticated,
    numericProblemId
  })
  const problemSubmitAction = useProblemSubmitAction({
    authState,
    isAuthenticated,
    numericProblemId,
    selectedLanguage,
    sourceCode,
    isLoadingLanguages: problemSubmitResource.isLoadingLanguages,
    isSubmissionBanActive: problemSubmitResource.isSubmissionBanActive,
    refreshSubmissionBan: problemSubmitResource.loadMySubmissionBan
  })
  const problemSubmitEditor = useProblemSubmitEditor({
    selectedLanguage,
    sourceCode,
    sourceEditorElement,
    supportedLanguages: problemSubmitResource.supportedLanguages,
    isSubmittingSubmission: problemSubmitAction.isSubmittingSubmission,
    onSubmit: problemSubmitAction.submitSolution
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
    selectedLanguage,
    sourceCode,
    sourceEditorElement,
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

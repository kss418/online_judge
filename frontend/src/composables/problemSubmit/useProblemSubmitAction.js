import { computed, ref } from 'vue'
import { useRouter } from 'vue-router'

import { createSubmission } from '@/api/submission'
import { formatApiError } from '@/utils/apiError'
import { formatTimestamp } from '@/utils/dateTime'

const submissionFieldLabels = {
  language: '언어',
  source_code: '소스 코드'
}

export function useProblemSubmitAction({
  authState,
  isAuthenticated,
  numericProblemId,
  selectedLanguage,
  sourceCode,
  isLoadingLanguages,
  isSubmissionBanActive,
  refreshSubmissionBan
}){
  const router = useRouter()
  const isSubmittingSubmission = ref(false)
  const submitErrorMessage = ref('')
  const submissionSuccessMessage = ref('')
  const canSubmit = computed(() =>
    isAuthenticated.value &&
    Boolean(selectedLanguage.value) &&
    Boolean(sourceCode.value.trim()) &&
    !isSubmittingSubmission.value &&
    !isLoadingLanguages.value &&
    !isSubmissionBanActive.value
  )

  function resetSubmitFeedback(){
    submitErrorMessage.value = ''
    submissionSuccessMessage.value = ''
  }

  async function submitSolution(){
    if (!isAuthenticated.value || !authState.token) {
      submitErrorMessage.value = '제출하려면 로그인하세요.'
      submissionSuccessMessage.value = ''
      return
    }

    if (!selectedLanguage.value) {
      submitErrorMessage.value = '언어를 선택하세요.'
      submissionSuccessMessage.value = ''
      return
    }

    if (!sourceCode.value.trim()) {
      submitErrorMessage.value = '소스 코드를 입력하세요.'
      submissionSuccessMessage.value = ''
      return
    }

    isSubmittingSubmission.value = true
    submitErrorMessage.value = ''
    submissionSuccessMessage.value = ''

    try {
      await createSubmission(
        numericProblemId.value,
        {
          language: selectedLanguage.value,
          source_code: sourceCode.value
        },
        authState.token
      )

      await router.push({
        name: 'problem-my-submissions',
        params: {
          problemId: String(numericProblemId.value)
        }
      })
    } catch (error) {
      if (error?.code === 'submission_banned') {
        const submissionBanStatus = await refreshSubmissionBan()
        const submissionBanUntil = submissionBanStatus?.data?.submission_banned_until

        if (submissionBanUntil) {
          submitErrorMessage.value = `제출이 ${formatTimestamp(submissionBanUntil)}까지 제한되어 있습니다.`
        } else {
          submitErrorMessage.value = '현재 제출이 제한되어 있습니다.'
        }
      } else {
        submitErrorMessage.value = formatApiError(error, {
          fallback: '제출을 처리하지 못했습니다.',
          fieldLabels: submissionFieldLabels
        })
      }
    } finally {
      isSubmittingSubmission.value = false
    }
  }

  return {
    isSubmittingSubmission,
    submitErrorMessage,
    submissionSuccessMessage,
    canSubmit,
    resetSubmitFeedback,
    submitSolution
  }
}

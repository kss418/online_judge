import { computed } from 'vue'

import { runBusyAction } from '@/composables/adminShared/runBusyAction'
import { problemBusySection } from '@/composables/adminProblems/problemBusySection'
import { updateProblemStatement } from '@/api/problemAdminApi'
import { formatApiError } from '@/utils/apiError'

export function useProblemStatementActions({
  authState,
  busySection,
  formatCount,
  selectedProblemDetail,
  updateSelectedProblemDetail,
  descriptionDraft,
  inputFormatDraft,
  outputFormatDraft,
  noteDraft,
  canSaveStatement,
  applySelectedProblemVersion,
  setActionFeedback
}){
  const isSavingStatement = computed(() => busySection.value === problemBusySection.SAVE_STATEMENT)
  const clearActionFeedback = () => setActionFeedback({
    message: '',
    error: ''
  })

  async function handleSaveStatement(){
    if (!authState.token || !selectedProblemDetail.value || !canSaveStatement.value) {
      return
    }

    const problemId = selectedProblemDetail.value.problem_id
    const nextStatement = {
      description: descriptionDraft.value,
      input_format: inputFormatDraft.value,
      output_format: outputFormatDraft.value,
      note: noteDraft.value === '' ? null : noteDraft.value
    }

    return runBusyAction({
      busySection,
      section: problemBusySection.SAVE_STATEMENT,
      clearFeedback: clearActionFeedback,
      run: async () => {
        const response = await updateProblemStatement(problemId, nextStatement, authState.token)

        updateSelectedProblemDetail((problemDetail) => problemDetail
          ? {
            ...problemDetail,
            statement: {
              description: descriptionDraft.value,
              input_format: inputFormatDraft.value,
              output_format: outputFormatDraft.value,
              note: noteDraft.value
            }
          }
          : problemDetail
        )
        applySelectedProblemVersion(problemId, response.version)
        setActionFeedback({
          message: `문제 #${formatCount(problemId)} 설명을 저장했습니다.`
        })
      },
      onError: (error) => {
        setActionFeedback({
          error: formatApiError(error, {
            fallback: '문제 설명을 저장하지 못했습니다.'
          })
        })
      }
    })
  }

  return {
    isSavingStatement,
    handleSaveStatement
  }
}

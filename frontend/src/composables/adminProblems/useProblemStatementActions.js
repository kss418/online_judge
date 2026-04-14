import { computed } from 'vue'

import { problemBusySection } from '@/composables/adminProblems/problemBusySection'
import { updateProblemStatement } from '@/api/problemAdminApi'
import { formatApiError } from '@/utils/apiError'

export function useProblemStatementActions({
  authState,
  busySection,
  formatCount,
  selectedProblemDetail,
  descriptionDraft,
  inputFormatDraft,
  outputFormatDraft,
  noteDraft,
  canSaveStatement,
  applySelectedProblemVersion,
  setActionFeedback
}){
  const isSavingStatement = computed(() => busySection.value === problemBusySection.SAVE_STATEMENT)

  async function handleSaveStatement(){
    if (!authState.token || !selectedProblemDetail.value || !canSaveStatement.value) {
      return
    }

    const problemId = selectedProblemDetail.value.problem_id
    busySection.value = problemBusySection.SAVE_STATEMENT
    setActionFeedback({
      message: '',
      error: ''
    })

    try {
      const nextStatement = {
        description: descriptionDraft.value,
        input_format: inputFormatDraft.value,
        output_format: outputFormatDraft.value,
        note: noteDraft.value === '' ? null : noteDraft.value
      }
      const response = await updateProblemStatement(problemId, nextStatement, authState.token)

      selectedProblemDetail.value = {
        ...selectedProblemDetail.value,
        statement: {
          description: descriptionDraft.value,
          input_format: inputFormatDraft.value,
          output_format: outputFormatDraft.value,
          note: noteDraft.value
        }
      }
      applySelectedProblemVersion(problemId, response.version)
      setActionFeedback({
        message: `문제 #${formatCount(problemId)} 설명을 저장했습니다.`
      })
    } catch (error) {
      setActionFeedback({
        error: formatApiError(error, {
          fallback: '문제 설명을 저장하지 못했습니다.'
        })
      })
    } finally {
      busySection.value = ''
    }
  }

  return {
    isSavingStatement,
    handleSaveStatement
  }
}

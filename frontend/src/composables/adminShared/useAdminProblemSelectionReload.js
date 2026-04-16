import { parsePositiveInteger } from '@/utils/parse'

function normalizeProblemId(value){
  return parsePositiveInteger(value) ?? 0
}

export function useAdminProblemSelectionReload({
  pageAccess,
  selectedProblemId,
  resetSelectedProblemState,
  reloadSelectedProblemData
}){
  if (!pageAccess) {
    return
  }

  pageAccess.watchWhenAllowed(selectedProblemId, (problemId) => {
    resetSelectedProblemState()

    if (normalizeProblemId(problemId) > 0) {
      void reloadSelectedProblemData(problemId)
    }
  })
}

import { useAdminProblemSelectionQueryBase } from '@/composables/adminShared/useAdminProblemSelectionQueryBase'
import {
  buildRouteQuery as buildProblemAdminSearchRouteQuery,
  parseRouteQuery as parseProblemAdminSearchRouteQuery
} from '@/queryState/problemAdminSearch'

function resolveSelectedProblemId(selectedProblemId){
  if (selectedProblemId && typeof selectedProblemId === 'object' && 'value' in selectedProblemId) {
    return selectedProblemId.value
  }

  return selectedProblemId ?? 0
}

export function useAdminProblemCatalogQuery({
  route,
  router,
  formatCount,
  selectedProblemId,
  reloadProblems,
  showErrorNotice,
  includeSelectedProblemIdInQuery = false,
  buildLocation
}){
  return useAdminProblemSelectionQueryBase({
    route,
    router,
    formatCount,
    selectedProblemId,
    reloadProblems,
    showErrorNotice,
    parseQuery(query){
      return parseProblemAdminSearchRouteQuery(query, {
        includeSelectedProblemId: includeSelectedProblemIdInQuery
      })
    },
    buildQuery(state){
      return buildProblemAdminSearchRouteQuery({
        ...state,
        selectedProblemId: includeSelectedProblemIdInQuery
          ? resolveSelectedProblemId(selectedProblemId)
          : state.selectedProblemId
      }, {
        includeSelectedProblemId: includeSelectedProblemIdInQuery
      })
    },
    buildLocation
  })
}

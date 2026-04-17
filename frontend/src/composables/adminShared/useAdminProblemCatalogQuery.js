import { useAdminProblemSelectionQueryBase } from '@/composables/adminShared/useAdminProblemSelectionQueryBase'
import {
  buildRouteQuery as buildProblemAdminSearchRouteQuery,
  parseRouteQuery as parseProblemAdminSearchRouteQuery
} from '@/queryState/problemAdminSearch'

function resolveSelectedProblemId(selectedProblemId){
  if (selectedProblemId && typeof selectedProblemId === 'object' && 'value' in selectedProblemId) {
    const normalizedProblemId = Number(selectedProblemId.value)
    return Number.isInteger(normalizedProblemId) && normalizedProblemId > 0
      ? normalizedProblemId
      : 0
  }

  const normalizedProblemId = Number(selectedProblemId)
  return Number.isInteger(normalizedProblemId) && normalizedProblemId > 0
    ? normalizedProblemId
    : 0
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
          ? resolveSelectedProblemId(
            typeof state.selectedProblemId === 'undefined'
              ? selectedProblemId
              : state.selectedProblemId
          )
          : state.selectedProblemId
      }, {
        includeSelectedProblemId: includeSelectedProblemIdInQuery
      })
    },
    buildLocation
  })
}

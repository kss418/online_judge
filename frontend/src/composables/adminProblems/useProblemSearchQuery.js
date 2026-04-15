import { useAdminProblemSelectionQueryBase } from '@/composables/adminShared/useAdminProblemSelectionQueryBase'
import {
  buildRouteQuery as buildProblemAdminSearchRouteQuery,
  parseRouteQuery as parseProblemAdminSearchRouteQuery
} from '@/queryState/problemAdminSearch'

export function useProblemSearchQuery({
  route,
  router,
  formatCount,
  selectedProblemId,
  reloadProblems,
  showErrorNotice
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
        includeSelectedProblemId: true
      })
    },
    buildQuery(state){
      return buildProblemAdminSearchRouteQuery(state, {
        includeSelectedProblemId: true
      })
    }
  })
}

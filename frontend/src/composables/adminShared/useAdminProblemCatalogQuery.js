import { useAdminProblemSelectionQueryBase } from '@/composables/adminShared/useAdminProblemSelectionQueryBase'
import {
  buildRouteQuery as buildProblemAdminSearchRouteQuery,
  parseRouteQuery as parseProblemAdminSearchRouteQuery
} from '@/queryState/problemAdminSearch'

export function useAdminProblemCatalogQuery({
  route,
  router,
  formatCount,
  selectedProblemId,
  reloadProblems,
  showErrorNotice,
  buildLocation
}){
  return useAdminProblemSelectionQueryBase({
    route,
    router,
    formatCount,
    selectedProblemId,
    reloadProblems,
    showErrorNotice,
    parseQuery: parseProblemAdminSearchRouteQuery,
    buildQuery: buildProblemAdminSearchRouteQuery,
    buildLocation
  })
}

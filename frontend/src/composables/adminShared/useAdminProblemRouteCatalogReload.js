import { parsePositiveInteger } from '@/utils/parse'

function normalizeProblemId(value){
  return parsePositiveInteger(value) ?? 0
}

function resolveSourceValue(source){
  return source?.value
}

export function useAdminProblemRouteCatalogReload({
  pageAccess,
  sources,
  syncFromRoute,
  reloadCatalog,
  getPreferredProblemId
}){
  if (!pageAccess || !Array.isArray(sources) || sources.length === 0) {
    return
  }

  pageAccess.watchWhenAllowed(
    () => sources.map((source) => resolveSourceValue(source)),
    () => {
      syncFromRoute()
      void reloadCatalog(normalizeProblemId(getPreferredProblemId()))
    }
  )
}

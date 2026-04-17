export function useAdminProblemRouteCatalogReload({
  pageAccess,
  query
}){
  if (!pageAccess || !query) {
    return
  }

  return pageAccess.watchWhenAllowed(
    () => [
      query.routeSearchMode.value,
      query.routeTitleSearch.value,
      query.routeProblemIdSearch.value
    ],
    () => {
      void query.syncFromRouteAndReload()
    }
  )
}

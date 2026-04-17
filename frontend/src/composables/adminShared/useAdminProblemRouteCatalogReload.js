export function useAdminProblemRouteCatalogReload({
  pageAccess,
  query
}){
  if (!pageAccess || !query) {
    return
  }

  pageAccess.watchWhenAllowed(
    () => [
      query.routeSearchMode.value,
      query.routeTitleSearch.value,
      query.routeProblemIdSearch.value,
      query.preferredProblemIdFromRoute.value
    ],
    () => {
      void query.syncFromRouteAndReload()
    }
  )
}

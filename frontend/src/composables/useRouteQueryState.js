import { computed } from 'vue'

function normalizeQueryScalar(value){
  if (value === null || typeof value === 'undefined') {
    return undefined
  }

  if (typeof value === 'string') {
    const trimmedValue = value.trim()
    return trimmedValue ? trimmedValue : undefined
  }

  if (typeof value === 'number') {
    return Number.isFinite(value) ? String(value) : undefined
  }

  if (typeof value === 'boolean') {
    return value ? 'true' : 'false'
  }

  const normalizedValue = String(value).trim()
  return normalizedValue ? normalizedValue : undefined
}

function normalizeQueryValue(value){
  if (Array.isArray(value)) {
    for (const item of value) {
      const normalizedItem = normalizeQueryScalar(item)
      if (typeof normalizedItem !== 'undefined') {
        return normalizedItem
      }
    }

    return undefined
  }

  return normalizeQueryScalar(value)
}

function normalizeQueryObject(query){
  const nextQuery = {}

  Object.entries(query ?? {}).forEach(([key, value]) => {
    const normalizedValue = normalizeQueryValue(value)
    if (typeof normalizedValue !== 'undefined') {
      nextQuery[key] = normalizedValue
    }
  })

  return nextQuery
}

function areQueryValuesEqual(leftValue, rightValue){
  if (Array.isArray(leftValue) || Array.isArray(rightValue)) {
    const leftValues = Array.isArray(leftValue) ? leftValue : [leftValue]
    const rightValues = Array.isArray(rightValue) ? rightValue : [rightValue]

    if (leftValues.length !== rightValues.length) {
      return false
    }

    return leftValues.every((value, index) => value === rightValues[index])
  }

  return leftValue === rightValue
}

function areQueriesEqual(leftQuery, rightQuery){
  const leftKeys = Object.keys(leftQuery).sort()
  const rightKeys = Object.keys(rightQuery).sort()

  if (leftKeys.length !== rightKeys.length) {
    return false
  }

  return leftKeys.every((key, index) =>
    key === rightKeys[index] && areQueryValuesEqual(leftQuery[key], rightQuery[key])
  )
}

function normalizeLocation(location, fallbackRoute){
  return {
    name: Object.prototype.hasOwnProperty.call(location, 'name')
      ? location.name
      : fallbackRoute.name,
    params: Object.prototype.hasOwnProperty.call(location, 'params')
      ? normalizeQueryObject(location.params)
      : normalizeQueryObject(fallbackRoute.params),
    query: Object.prototype.hasOwnProperty.call(location, 'query')
      ? normalizeQueryObject(location.query)
      : normalizeQueryObject(fallbackRoute.query)
  }
}

function areLocationsEqual(leftLocation, rightLocation){
  return leftLocation.name === rightLocation.name &&
    areQueriesEqual(leftLocation.params, rightLocation.params) &&
    areQueriesEqual(leftLocation.query, rightLocation.query)
}

export function useRouteQueryState({
  route,
  router,
  parseQuery,
  buildQuery,
  createLocalState,
  syncLocalState,
  buildLocation
}){
  const localState = typeof createLocalState === 'function'
    ? createLocalState()
    : {}

  const normalizedRouteQuery = computed(() => normalizeQueryObject(route.query))
  const routeState = computed(() => parseQuery(normalizedRouteQuery.value))

  function buildCanonicalQuery(nextState){
    return normalizeQueryObject(buildQuery(nextState))
  }

  function syncFromRoute(){
    if (typeof syncLocalState === 'function') {
      syncLocalState(localState, routeState.value)
    }
  }

  function isCurrentState(nextState){
    return areQueriesEqual(
      normalizedRouteQuery.value,
      buildCanonicalQuery(nextState)
    )
  }

  function createLocation(nextState, options = {}){
    const mode = options.mode === 'push' ? 'push' : 'replace'
    const nextQuery = typeof options.query === 'undefined'
      ? buildCanonicalQuery(nextState)
      : normalizeQueryObject(options.query)
    const location = typeof buildLocation === 'function'
      ? (buildLocation({
        mode,
        nextState,
        query: nextQuery,
        routeState: routeState.value,
        selectedProblemId: options.selectedProblemId
      }) ?? {})
      : {
        query: nextQuery
      }

    if (!Object.prototype.hasOwnProperty.call(location, 'query')) {
      return {
        ...location,
        query: nextQuery
      }
    }

    return location
  }

  async function navigate(nextState, options = {}){
    const mode = options.mode === 'push' ? 'push' : 'replace'
    const location = createLocation(nextState, options)

    if (areLocationsEqual(
      normalizeLocation(route, route),
      normalizeLocation(location, route)
    )) {
      return false
    }

    await router[mode](location)
    return true
  }

  syncFromRoute()

  return {
    routeState,
    localState,
    syncFromRoute,
    navigate,
    isCurrentState,
    buildCanonicalQuery,
    createLocation
  }
}

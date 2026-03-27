export function buildPaginationItems(currentPage, totalPages){
  const pages = new Set([
    1,
    totalPages,
    currentPage - 1,
    currentPage,
    currentPage + 1
  ])

  const sortedPages = Array.from(pages)
    .filter((pageNumber) => pageNumber >= 1 && pageNumber <= totalPages)
    .sort((left, right) => left - right)

  const items = []

  sortedPages.forEach((pageNumber, index) => {
    const previousPage = sortedPages[index - 1]

    if (index > 0) {
      if (pageNumber - previousPage === 2) {
        items.push({
          type: 'page',
          value: previousPage + 1,
          key: `page-${previousPage + 1}`
        })
      } else if (pageNumber - previousPage > 2) {
        items.push({
          type: 'ellipsis',
          key: `ellipsis-${previousPage}-${pageNumber}`
        })
      }
    }

    items.push({
      type: 'page',
      value: pageNumber,
      key: `page-${pageNumber}`
    })
  })

  return items
}

import { noticeStore } from '@/stores/notice/noticeStore'

const noticeComposable = {
  noticeState: noticeStore.state,
  showNotice: noticeStore.showNotice,
  showSuccessNotice: noticeStore.showSuccessNotice,
  showErrorNotice: noticeStore.showErrorNotice,
  clearNotice: noticeStore.clearNotice
}

export function useNotice(){
  return noticeComposable
}

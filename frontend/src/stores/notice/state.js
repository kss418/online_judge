import { reactive, readonly } from 'vue'

const noticeState = reactive({
  items: []
})

let nextNoticeId = 1

export const state = readonly(noticeState)

export function pushNotice(notice){
  noticeState.items.unshift(notice)
}

export function removeNotice(noticeId){
  noticeState.items = noticeState.items.filter((item) => item.id !== noticeId)
}

export function clearNoticeItems(){
  noticeState.items = []
}

export function allocateNoticeId(){
  return nextNoticeId++
}

<template>
  <TransitionGroup
    name="global-notice"
    tag="div"
    class="global-notice-region"
    :aria-live="hasDangerNotice ? 'assertive' : 'polite'"
  >
    <div
      v-for="notice in noticeState.items"
      :key="notice.id"
      class="global-notice"
      :class="`is-${notice.tone || 'neutral'}`"
      :role="notice.tone === 'danger' ? 'alert' : 'status'"
    >
      <p>{{ notice.message }}</p>
      <button
        type="button"
        class="global-notice-close"
        @click="clearNotice(notice.id)"
      >
        닫기
      </button>
    </div>
  </TransitionGroup>
</template>

<script setup>
import { computed } from 'vue'

import { noticeStore } from '@/stores/notice/noticeStore'

const {
  state: noticeState,
  clearNotice
} = noticeStore
const hasDangerNotice = computed(() =>
  noticeState.items.some((notice) => notice.tone === 'danger')
)
</script>

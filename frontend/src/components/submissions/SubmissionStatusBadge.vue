<template>
  <span
    class="status-badge submission-status-badge"
    :data-tone="resolvedTone"
  >
    {{ resolvedLabel }}
  </span>
</template>

<script setup>
import { computed } from 'vue'

import {
  getSubmissionStatusLabel,
  getSubmissionStatusTone
} from '@/generated/submissionStatusCatalog'

const props = defineProps({
  status: {
    type: String,
    required: true
  },
  label: {
    type: String,
    default: ''
  }
})

const normalizedStatus = computed(() =>
  typeof props.status === 'string'
    ? props.status.trim()
    : ''
)

const resolvedLabel = computed(() =>
  props.label || getSubmissionStatusLabel(normalizedStatus.value)
)

const resolvedTone = computed(() =>
  getSubmissionStatusTone(normalizedStatus.value)
)
</script>

<style scoped>
.submission-status-badge {
  white-space: nowrap;
}
</style>

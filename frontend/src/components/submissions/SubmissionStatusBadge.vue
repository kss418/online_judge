<template>
  <span
    class="submission-status-badge"
    :data-status="normalizedStatus"
  >
    {{ resolvedLabel }}
  </span>
</template>

<script setup>
import { computed } from 'vue'

import { getSubmissionStatusLabel } from '@/utils/submissionStatus'

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
</script>

<style scoped>
.submission-status-badge {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-height: 2rem;
  padding: 0.35rem 0.78rem;
  border-radius: 999px;
  border: 1px solid rgba(148, 163, 184, 0.2);
  background: rgba(248, 250, 252, 0.95);
  color: var(--ink-strong);
  font-size: 0.84rem;
  font-weight: 700;
  line-height: 1.1;
  white-space: nowrap;
}

.submission-status-badge[data-status='queued'] {
  color: #475569;
  background: rgba(226, 232, 240, 0.9);
  border-color: rgba(100, 116, 139, 0.22);
}

.submission-status-badge[data-status='judging'] {
  color: #0f766e;
  background: rgba(204, 251, 241, 0.92);
  border-color: rgba(13, 148, 136, 0.24);
}

.submission-status-badge[data-status='accepted'] {
  color: #15803d;
  background: rgba(220, 252, 231, 0.92);
  border-color: rgba(22, 163, 74, 0.24);
}

.submission-status-badge[data-status='wrong_answer'] {
  color: #b91c1c;
  background: rgba(254, 226, 226, 0.94);
  border-color: rgba(220, 38, 38, 0.24);
}

.submission-status-badge[data-status='time_limit_exceeded'] {
  color: #b45309;
  background: rgba(254, 243, 199, 0.95);
  border-color: rgba(217, 119, 6, 0.24);
}

.submission-status-badge[data-status='memory_limit_exceeded'] {
  color: #c2410c;
  background: rgba(255, 237, 213, 0.95);
  border-color: rgba(234, 88, 12, 0.24);
}

.submission-status-badge[data-status='runtime_error'] {
  color: #7c3aed;
  background: rgba(237, 233, 254, 0.95);
  border-color: rgba(124, 58, 237, 0.22);
}

.submission-status-badge[data-status='compile_error'] {
  color: #1d4ed8;
  background: rgba(219, 234, 254, 0.95);
  border-color: rgba(37, 99, 235, 0.22);
}

.submission-status-badge[data-status='output_exceeded'] {
  color: #be185d;
  background: rgba(252, 231, 243, 0.95);
  border-color: rgba(219, 39, 119, 0.22);
}
</style>

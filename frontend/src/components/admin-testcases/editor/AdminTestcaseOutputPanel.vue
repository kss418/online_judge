<template>
  <label class="field-block">
    <span class="field-label">출력</span>
    <textarea
      :value="selectedTestcaseOutputDraft"
      class="admin-testcases-textarea"
      spellcheck="false"
      :disabled="Boolean(busySection)"
      placeholder="빈 출력도 허용됩니다."
      @input="$emit('update:selectedTestcaseOutputDraft', $event.target.value)"
    />
  </label>
</template>

<script setup>
defineProps({
  selectedTestcaseOutputDraft: {
    type: String,
    default: ''
  },
  busySection: {
    type: String,
    default: ''
  }
})

defineEmits(['update:selectedTestcaseOutputDraft'])
</script>

<style scoped>
.admin-testcases-textarea {
  width: 100%;
  min-height: 12rem;
  padding: 0.9rem 1rem;
  border-radius: 18px;
  border: 1px solid var(--line);
  background: var(--admin-testcases-nested-surface);
  color: var(--ink-strong);
  font: inherit;
  resize: vertical;
  line-height: 1.6;
}

.admin-testcases-textarea:focus {
  outline: 2px solid rgba(217, 119, 6, 0.18);
  border-color: rgba(217, 119, 6, 0.5);
}
</style>

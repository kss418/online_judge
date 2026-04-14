<template>
  <label class="field-block">
    <span class="field-label">입력</span>
    <textarea
      :value="section.model.selectedTestcaseInputDraft"
      class="admin-testcases-textarea"
      spellcheck="false"
      :disabled="Boolean(section.model.busySection)"
      placeholder="빈 입력도 허용됩니다."
      @input="handleSelectedTestcaseInputDraft"
    />
  </label>
</template>

<script setup>
import { computed } from 'vue'

const props = defineProps({
  section: {
    type: Object,
    required: true
  }
})

const section = computed(() => props.section)

function handleSelectedTestcaseInputDraft(event){
  section.value.actions.updateSelectedTestcaseInputDraft(event.target.value)
}
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

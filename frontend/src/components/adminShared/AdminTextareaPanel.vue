<template>
  <label class="field-block">
    <span class="field-label">{{ label }}</span>
    <textarea
      :value="modelValue"
      class="admin-textarea-panel__textarea"
      spellcheck="false"
      :disabled="disabled"
      :placeholder="placeholder"
      @input="$emit('update:modelValue', $event.target.value)"
    />
  </label>
</template>

<script setup>
defineProps({
  modelValue: {
    type: String,
    default: ''
  },
  label: {
    type: String,
    required: true
  },
  placeholder: {
    type: String,
    default: ''
  },
  disabled: {
    type: Boolean,
    default: false
  }
})

defineEmits(['update:modelValue'])
</script>

<style scoped>
.admin-textarea-panel__textarea {
  width: 100%;
  min-height: 12rem;
  padding: 0.9rem 1rem;
  border-radius: 18px;
  border: 1px solid var(--line);
  background: var(--admin-workspace-nested-surface);
  color: var(--ink-strong);
  font: inherit;
  resize: vertical;
  line-height: 1.6;
}

.admin-textarea-panel__textarea:focus {
  outline: 2px solid rgba(217, 119, 6, 0.18);
  border-color: rgba(217, 119, 6, 0.5);
}
</style>

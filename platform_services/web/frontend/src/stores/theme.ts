import { defineStore } from 'pinia'
import { ref } from 'vue'

const STORAGE_KEY = 'lgtbot_theme'

export const useThemeStore = defineStore('theme', () => {
  const isDark = ref(false)

  function apply() {
    document.documentElement.classList.toggle('dark', isDark.value)
    try {
      localStorage.setItem(STORAGE_KEY, isDark.value ? 'dark' : 'light')
    } catch {
      /* ignore */
    }
  }

  function init() {
    try {
      const saved = localStorage.getItem(STORAGE_KEY)
      isDark.value = saved === 'dark'
    } catch {
      isDark.value = false
    }
    apply()
  }

  function toggle() {
    isDark.value = !isDark.value
    apply()
  }

  return { isDark, init, toggle }
})

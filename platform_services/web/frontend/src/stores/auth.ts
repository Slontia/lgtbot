import { defineStore } from 'pinia'
import { computed, ref } from 'vue'

const TOKEN_KEY = 'lgtbot_web_token'
const USER_KEY = 'lgtbot_web_username'

export const useAuthStore = defineStore('auth', () => {
  const token = ref<string | null>(localStorage.getItem(TOKEN_KEY))
  const username = ref<string | null>(localStorage.getItem(USER_KEY))

  const isLoggedIn = computed(() => !!token.value)

  function setSession(t: string, user: string) {
    token.value = t
    username.value = user
    localStorage.setItem(TOKEN_KEY, t)
    localStorage.setItem(USER_KEY, user)
  }

  function logout() {
    token.value = null
    username.value = null
    localStorage.removeItem(TOKEN_KEY)
    localStorage.removeItem(USER_KEY)
  }

  return { token, username, isLoggedIn, setSession, logout }
})

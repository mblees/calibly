.pragma library

// Times as a player shows them, from milliseconds:
//   clock(372000)   -> "6:12"
//   clock(51727000) -> "14:22:07"
//   clock(372000, true) -> "0:06:12" (always with hours)
function clock(ms, withHours) {
    const total = Math.max(0, Math.floor(ms / 1000))
    const hours = Math.floor(total / 3600)
    const minutes = Math.floor((total % 3600) / 60)
    const seconds = total % 60
    const two = n => (n < 10 ? "0" : "") + n
    if (hours > 0 || withHours)
        return hours + ":" + two(minutes) + ":" + two(seconds)
    return minutes + ":" + two(seconds)
}

// Remaining time with a minus sign, e.g. "−8:41".
function remaining(ms) {
    return "−" + clock(ms)
}

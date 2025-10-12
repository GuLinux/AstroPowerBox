export const historyEntryTimestamp = (uptime, entryUptime) => {
    const dateStarted = new Date().getTime() - (uptime* 1000);
    return dateStarted + entryUptime
}
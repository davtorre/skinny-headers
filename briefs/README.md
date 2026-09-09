# Briefs

This directory holds task briefs for agents working on skinny-headers.
The project manager (Claude, this session) authors each brief; the user
fans agents out to execute them.

## Workflow
1. PM writes a brief here as `NNN-short-slug.md` (zero-padded, sequential).
2. User dispatches an agent against the brief.
3. If the agent needs to reach the PM (question, blocker, handoff, or
   deliverable report), it does so under the brief's **Deliverable**
   section instructions. The PM logs every such exchange — in both
   directions — in `../agent_comms.csv`.

## Brief template
```markdown
# Brief NNN: <title>

## Context
<what the agent needs to know: module, files, prior state>

## Task
<the concrete work to do>

## Constraints
<style conventions, things not to touch, CLAUDE.md rules that apply>

## Deliverable
<what "done" looks like, where output should land>

If you need to reach the project manager (question, blocker, or status
report), send the message and note that it should be logged in
`agent_comms.csv`.
```

## agent_comms.csv columns
`timestamp,brief,agent,direction,message`
- `timestamp`: ISO 8601
- `brief`: brief filename (e.g. `001-dat-list-nesting.md`)
- `agent`: agent name/id reporting in
- `direction`: `agent->pm` or `pm->agent`
- `message`: free text (keep to one line; escape commas/quotes per CSV rules)

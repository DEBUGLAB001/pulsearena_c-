const $ = (id) => document.getElementById(id);

let currentState = null;
let currentVersion = null;
let isBusy = false;

const initials = (id) => id.slice(0, 2);

const getTeamName = (id, state) => {
  for (const group of state.groups) {
    for (const team of group.teams) {
      if (team.id === id) {
        return team.name;
      }
    }
  }
  if (id.startsWith('winner-')) {
    return 'Winner · ' + id.slice(7);
  }
  return id || 'TBD';
};

function renderStats(state) {
  const matchesPlayed = state.matches.filter((m) => m.played).length;
  const allTeams = state.groups.flatMap((g) => g.teams);
  const leader = state.leader;

  const leaderDiffStr = leader
    ? `${leader.points} pts · ${leader.diff > 0 ? '+' : ''}${leader.diff} diff`
    : 'Waiting for results';

  const cards = [
    ['TEAMS IN PLAY', allTeams.length, 'Across qualification groups', '#69d9e8'],
    ['MATCHES PLAYED', `${matchesPlayed} / ${state.matchCount}`, 'Group stage fixtures', '#a9f36c'],
    ['PLAYOFF PLACES', state.qualifierCount, 'Top teams advance', '#b09cff'],
    ['CURRENT LEADER', leader?.id ?? '—', leaderDiffStr, '#ffb86b']
  ];

  $('stats').innerHTML = cards
    .map(
      ([label, value, foot, color]) => `
      <div class="stat-card" style="--glow:${color}">
        <div class="stat-label">${label}</div>
        <div class="stat-value">${value}</div>
        <div class="stat-foot">${foot}</div>
      </div>`
    )
    .join('');
}

function renderGroups(state) {
  $('groupsContent').innerHTML = state.groups
    .map((group) => {
      const qualifyingCount = group.teams.filter((_, i) => i < 2).length;
      const teamRows = group.teams
        .map(
          (team, i) => `
        <div class="team-row ${i < 2 ? 'qualify' : ''}">
          <span class="seed">${String(i + 1).padStart(2, '0')}</span>
          <span class="team-name" title="${team.name}">${team.name}</span>
          <span class="team-meta">${team.wins}W</span>
          <span class="team-pts">${team.points} P</span>
        </div>`
        )
        .join('');

      return `
      <div class="group-card">
        <div class="group-title">
          Group ${group.name}
          <span>${qualifyingCount} QUALIFY</span>
        </div>
        ${teamRows}
      </div>`;
    })
    .join('');
}

function renderMatches(state) {
  const filter = $('groupFilter').value;
  const matches = state.matches.filter((m) => filter === 'all' || m.group === filter);

  if (matches.length === 0) {
    $('matchesContent').innerHTML = '<div class="subtle">No fixtures in this group.</div>';
    return;
  }

  $('matchesContent').innerHTML = matches
    .map((match, i) => {
      const home = getTeamName(match.home, state);
      const away = getTeamName(match.away, state);
      const timeSlot = `${String(i + 1).padStart(2, '0')} : 30`;

      return `
      <div class="match-card">
        <div class="match-time">${match.group}<br>${timeSlot}</div>
        <div class="match-pair">
          <div class="match-team">
            <strong>${home}</strong>
            <em>${match.played ? match.homeScore : '—'}</em>
          </div>
          <div class="versus">VS · ${match.id}</div>
          <div class="match-team">
            <strong>${away}</strong>
            <em>${match.played ? match.awayScore : '—'}</em>
          </div>
        </div>
        <button class="match-action ${match.played ? 'done' : ''}" data-match="${match.id}" data-winner="${match.home}">
          ${match.played ? '↻ Retry result' : 'Record result'}
        </button>
      </div>`;
    })
    .join('');

  document.querySelectorAll('[data-match]').forEach((button) => {
    button.addEventListener('click', () => submitResult(button.dataset.match, button.dataset.winner));
  });
}

function renderBracket(state) {
  const rounds = ['Opening round', 'Semifinal', 'Final'];

  $('bracketContent').innerHTML = rounds
    .map((round) => {
      const slots = state.bracket.filter((slot) => slot.round === round);
      const slotMarkup = slots
        .map((slot) => {
          const teamA = getTeamName(slot.teamA, state);
          const teamB = getTeamName(slot.teamB, state);
          const isWinnerA = slot.winner === slot.teamA ? '✓' : '';
          const isWinnerB = slot.winner === slot.teamB ? '✓' : '';

          return `
          <div class="bracket-match ${slot.byeA ? 'bye' : ''}">
            <div class="bracket-id">
              <span>${slot.matchId}</span>
              ${slot.byeA ? '<span class="bye-label">BYE ADVANTAGE</span>' : ''}
            </div>
            <div class="bracket-team ${slot.byeA ? 'seeded' : ''}">
              <span>${teamA}</span>
              <span>${isWinnerA}</span>
            </div>
            <div class="bracket-team">
              <span>${teamB}</span>
              <span>${isWinnerB}</span>
            </div>
          </div>`;
        })
        .join('');

      return `
      <div class="round-col">
        <div class="round-name">${round}</div>
        ${slotMarkup || '<div class="subtle">Bracket updates as teams qualify.</div>'}
      </div>`;
    })
    .join('');
}

function render(state) {
  currentState = state;
  renderStats(state);
  renderGroups(state);
  renderMatches(state);
  renderBracket(state);

  $('syncTime').textContent = new Date().toLocaleTimeString([], {
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit'
  });
  $('connection').textContent = '';
}

async function refresh() {
  if (isBusy) return;
  isBusy = true;
  try {
    const response = await fetch('api/state', { cache: 'no-store' });
    if (!response.ok) {
      throw new Error('State request failed');
    }
    render(await response.json());
  } catch (error) {
    $('connection').textContent = 'Reconnecting to tournament service…';
  } finally {
    isBusy = false;
  }
}

async function checkVersion() {
  try {
    const response = await fetch('api/version', { cache: 'no-store' });
    const data = await response.json();
    if (currentVersion === null) {
      currentVersion = data.version;
    } else if (currentVersion !== data.version) {
      location.reload();
    }
  } catch (_) {
    // Retry on the next poll; backend restarts are brief.
  }
}

async function submitResult(matchId, winner) {
  try {
    const body = new URLSearchParams({ matchId, winner });
    const response = await fetch('api/result', {
      method: 'POST',
      body,
      cache: 'no-store'
    });
    if (!response.ok) {
      throw new Error('Could not record this result');
    }
    await refresh();
  } catch (error) {
    $('connection').textContent = error.message;
  }
}

$('refreshBtn').addEventListener('click', refresh);

$('resetBtn').addEventListener('click', async () => {
  await fetch('api/reset', { method: 'POST', cache: 'no-store' });
  await refresh();
});

$('groupFilter').addEventListener('change', () => {
  if (currentState) {
    renderMatches(currentState);
  }
});

refresh();
checkVersion();

setInterval(refresh, 1800);
setInterval(checkVersion, 2200);


// Clay settings page for Facilitate Fitness 2.
//
// Keys must match `messageKeys` in package.json and the MESSAGE_KEY_* reads in
// src/c/settings.c. Defaults repeat set_defaults() there, so the page opens
// showing what the watch is actually doing on a first run. Theme values are
// indexes into the ThemeId enum (src/c/theme.h) and must never be reordered.

module.exports = [
  {
    type: 'heading',
    defaultValue: 'Facilitate Fitness 2',
    size: 1
  },
  {
    type: 'text',
    defaultValue:
      'Your day at a glance: steps against your goal, heart rate (Pebble ' +
      'Time 2), calories, distance, active minutes and last night\'s sleep, ' +
      'all from Pebble Health. Make sure Health is turned on in the Pebble app.'
  },

  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Look' },
      {
        type: 'select',
        messageKey: 'Theme',
        label: 'Theme',
        defaultValue: '0',
        options: [
          { label: 'Classic (lavender)', value: '0' },
          { label: 'Midnight (black)', value: '1' },
          { label: 'Blush (pink)', value: '2' }
        ]
      }
    ]
  },

  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Steps' },
      {
        type: 'select',
        messageKey: 'Goal',
        label: 'Daily step goal',
        description:
          'One lap of the ring is your goal. "My usual day" uses the steps ' +
          'Pebble Health says you normally do on this kind of day. The small ' +
          'tick on the ring shows where you usually are by now.',
        defaultValue: '10000',
        options: [
          { label: 'My usual day', value: '0' },
          { label: '5,000', value: '5000' },
          { label: '6,000', value: '6000' },
          { label: '7,500', value: '7500' },
          { label: '8,000', value: '8000' },
          { label: '10,000', value: '10000' },
          { label: '12,000', value: '12000' },
          { label: '15,000', value: '15000' },
          { label: '20,000', value: '20000' }
        ]
      },
      {
        type: 'select',
        messageKey: 'Units',
        label: 'Distance',
        defaultValue: '0',
        options: [
          { label: 'Same as Pebble Health', value: '0' },
          { label: 'Kilometres', value: '1' },
          { label: 'Miles', value: '2' }
        ]
      }
    ]
  },

  {
    type: 'submit',
    defaultValue: 'Save'
  }
];

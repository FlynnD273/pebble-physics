module.exports = [
  {
    "type": "heading",
    "defaultValue": "App Configuration"
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "General Settings"
      },
      {
        "type": "slider",
        "messageKey": "BALL_COUNT",
        "label": "Number of balls",
        "defaultValue": 20,
        "min": 1,
        "max": 255
      },
      {
        "type": "slider",
        "messageKey": "FPS",
        "label": "Target FPS",
        "defaultValue": 30,
        "min": 1,
        "max": 60
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];

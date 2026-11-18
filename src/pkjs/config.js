module.exports = [
  {
    type: "heading",
    defaultValue: "Fish Time"
  },
  {
    type: "text",
    defaultValue:
      "White background only. Default: full-color fish. Preview cycles all 44 species every 5 seconds."
  },
  {
    type: "toggle",
    messageKey: "ColorFish",
    label: "Color illustrations",
    description: "Off uses the original black-and-white plates.",
    defaultValue: true
  },
  {
    type: "toggle",
    messageKey: "PreviewCycle",
    label: "Preview all species (5 sec)",
    description: "Advances through each fish every 5 seconds (ignores daily rotation).",
    defaultValue: true
  },
  {
    type: "submit",
    defaultValue: "Save"
  }
];
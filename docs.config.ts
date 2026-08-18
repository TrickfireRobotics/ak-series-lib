import { defineConfig } from "trickfire-docs";

export default defineConfig({
  name: "TrickFire AK Series Driver",
  description:
    "Development guide and low level code documentation for the CubeMars AK Series driver board's. A deep dive into the code that controls our motors and how to contribute.",
  landing: [
    {
      title: "Getting Started",
      description: "Clone the repo and start developing",
      link: "/setup/getting-started/",
    },
    {
      title: "Development guides",
      description:
        "A link to some documents pulled from the manufacturer to get a better understanding of how the protocols work",
      link: "/guides/overview/",
    },
    {
      title: "Reference",
      description: "A deep dive into the code, the different API",
      link: "/reference/overview/",
    },
  ],
  sidebar: [
    {
      label: "Setup",
      items: [
        { label: "Getting Started", slug: "setup/getting-started" },
        { label: "Dev Container", slug: "setup/dev-container" },
      ],
    },
    {
      label: "Guides",
      items: [
        { label: "Protocol Overview", slug: "guides/overview" },
        { label: "Servo Mode", slug: "guides/servo-mode" },
        { label: "MIT Mode", slug: "guides/mit-mode" },
      ],
    },
    {
      label: "Reference",
      items: [
        { label: "Code Overview", slug: "reference/overview" },
        { label: "CAN Layer", slug: "reference/can-layer" },
        { label: "Motor Limits", slug: "reference/motors" },
        { label: "Motor Interface", slug: "reference/interface" },
      ],
    },
  ],
});

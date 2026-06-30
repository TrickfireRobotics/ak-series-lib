import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';

export default defineConfig({
    site: 'https://docs.trickfirerobotics.com',
    base: '/ak-series-lib',
    srcDir: './',
    integrations: [
        starlight({
            title: 'TrickFire AK Series Driver',
            head: [
                {
                    tag: 'script',
                    content: `
                        if (!localStorage.getItem('starlight-theme')) {
                            localStorage.setItem('starlight-theme', 'dark');
                        }
                    `
                }
            ],
            logo: {
                src: './assets/nav-logo.png',
                alt: 'TrickFire Robotics Logo',
                replacesTitle: true
            },
            favicon: '/favicon.ico',
            social: [
                {
                    icon: 'github',
                    label: 'GitHub',
                    href: 'https://github.com/TrickfireRobotics/'
                },
                {
                    icon: 'external',
                    label: 'Notion',
                    href: 'https://www.notion.so/trickfire/Drivebase-Team-1441fd41ff5b8015981cfc3d141d74cd?source=copy_link'
                },
                {
                    icon: 'external',
                    label: 'TrickFire Robotics',
                    href: 'https://.trickfirerobotics.com'
                }
            ],
            sidebar: [
                {
                    label: 'Setup',
                    items: [
                        { label: 'Getting Started', slug: 'setup/getting-started' },
                        { label: 'Dev Container', slug: 'setup/dev-container' }
                    ]
                },
                {
                    label: 'Guides',
                    items: [
                        { label: 'Protocol Overview', slug: 'guides/overview' },
                        { label: 'Servo Mode', slug: 'guides/servo-mode' },
                        { label: 'MIT Mode', slug: 'guides/mit-mode' },
                        { label: 'Contributing', slug: 'guides/contributing' }
                    ]
                },
                {
                    label: 'Reference',
                    items: [
                        { label: 'Code Overview', slug: 'reference/overview' },
                        { label: 'CAN Layer', slug: 'reference/can-layer' },
                        { label: 'Motor Limits', slug: 'reference/motors' }
                    ]
                }
            ],
            components: {
                SocialIcons: './components/SocialIcons.astro'
            },
            customCss: ['./styles/custom.css']
        })
    ]
});

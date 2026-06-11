import './globals.css';
import Sidebar from '@/components/Sidebar';
import ProgressBar from '@/components/ProgressBar';
import KonamiCode from '@/components/KonamiCode';

export const metadata = {
  title: '100 Days of Arduino',
  description: 'A practical Arduino curriculum from first circuits to robotics, control systems, and embedded autonomy.',
  metadataBase: new URL('https://paneendrakumar0.github.io/100-Projects-with-Arduino'),
  openGraph: {
    title: '100 Days of Arduino Masterclass',
    description: 'A practical Arduino curriculum from first circuits to robotics, control systems, and embedded autonomy. Build 100 hardware projects from scratch.',
    type: 'website',
    url: 'https://paneendrakumar0.github.io/100-Projects-with-Arduino',
    siteName: '100 Days of Arduino',
  },
  twitter: {
    card: 'summary_large_image',
    title: '100 Days of Arduino Masterclass',
    description: 'A practical Arduino curriculum from first circuits to robotics, control systems, and embedded autonomy.',
  },
};

export default function RootLayout({ children }) {
  return (
    <html lang="en">
      <body>
        <KonamiCode />
        <div className="noise-overlay"></div>
        <ProgressBar />
        <div className="app-container">
          <Sidebar />
          <main className="main-content">
            {children}
          </main>
        </div>
      </body>
    </html>
  );
}

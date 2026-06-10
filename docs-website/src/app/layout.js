import './globals.css';
import Sidebar from '@/components/Sidebar';
import ProgressBar from '@/components/ProgressBar';

export const metadata = {
  title: '100 Days of Arduino',
  description: 'The ultimate zero-to-hero journey for embedded systems.',
};

export default function RootLayout({ children }) {
  return (
    <html lang="en">
      <body>
        <div className="noise-overlay"></div>
        <div className="ambient-light"></div>
        <div className="ambient-light-2"></div>
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

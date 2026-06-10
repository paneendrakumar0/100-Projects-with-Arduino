import { getLearnArduinoChapters, get100DaysLogs } from '@/lib/markdown';
import { getSearchIndex } from '@/lib/searchData';
import SidebarClient from './SidebarClient';

export default function Sidebar() {
  const chapters = getLearnArduinoChapters();
  const days = get100DaysLogs();
  const searchIndex = getSearchIndex();

  return <SidebarClient chapters={chapters} days={days} searchIndex={searchIndex} />;
}

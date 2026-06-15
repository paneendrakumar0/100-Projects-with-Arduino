import { getHardwareGlossary } from '@/lib/markdown';
import HardwareGrid from '@/components/HardwareGrid';

export const metadata = {
  title: 'Hardware Glossary | 100 Days of Arduino'
};

export default function HardwarePage() {
  const glossary = getHardwareGlossary();
  
  return (
    <div className="max-w-6xl mx-auto py-12 px-6">
      <div className="mb-12">
        <h1 className="text-4xl font-black mb-4 bg-gradient-to-r from-cyan-400 to-blue-500 text-transparent bg-clip-text inline-block">
          Hardware Component Glossary
        </h1>
        <p className="text-zinc-400 text-lg max-w-2xl">
          Have a component sitting on your desk? Find out exactly which days use it across the entire 100 Days of Arduino curriculum. Click on any component to see all projects that require it.
        </p>
      </div>
      
      <HardwareGrid glossary={glossary} />
    </div>
  );
}

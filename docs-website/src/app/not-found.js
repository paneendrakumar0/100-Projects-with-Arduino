import Link from 'next/link';
import { AlertCircle, Home, Cpu } from 'lucide-react';

export default function NotFound() {
  return (
    <div className="min-h-[80vh] flex flex-col items-center justify-center px-6 text-center">
      <div className="relative mb-8 group">
        <div className="absolute inset-0 bg-red-500/20 blur-2xl rounded-full group-hover:bg-red-500/30 transition-all duration-700"></div>
        <div className="w-32 h-32 bg-zinc-900 border-2 border-red-500/50 rounded-3xl flex items-center justify-center relative shadow-2xl rotate-3 group-hover:rotate-6 transition-transform duration-500">
          <Cpu size={64} className="text-red-400" />
          <div className="absolute -bottom-4 -right-4 bg-red-500 text-white p-2 rounded-full shadow-lg">
            <AlertCircle size={24} />
          </div>
        </div>
      </div>
      
      <h1 className="text-5xl md:text-7xl font-black mb-4 tracking-tight bg-gradient-to-br from-red-400 to-red-600 text-transparent bg-clip-text">
        404
      </h1>
      
      <h2 className="text-2xl md:text-3xl font-bold text-zinc-100 mb-6">
        Sensor Not Found
      </h2>
      
      <p className="text-lg text-zinc-400 max-w-lg mb-10 leading-relaxed">
        We searched the entire I2C bus, but couldn't find the page you're looking for. It might have been unplugged or moved to a different address.
      </p>
      
      <div className="flex gap-4">
        <Link 
          href="/" 
          className="inline-flex items-center gap-2 px-6 py-3 bg-zinc-800 hover:bg-zinc-700 text-zinc-100 font-medium rounded-xl transition-all border border-zinc-700 hover:border-zinc-500 hover:shadow-lg"
        >
          <Home size={18} />
          Return to Base
        </Link>
      </div>
    </div>
  );
}

'use client';

import React, { useState, useEffect } from 'react';
import { Award, Printer, Lock } from 'lucide-react';
import Link from 'next/link';
import TwitterShareButton from '@/components/TwitterShareButton';

export default function CertificatePage() {
  const [completedDays, setCompletedDays] = useState([]);
  const [name, setName] = useState('');
  const [isClient, setIsClient] = useState(false);

  useEffect(() => {
    // eslint-disable-next-line react-hooks/set-state-in-effect
    setIsClient(true);
    const stored = JSON.parse(localStorage.getItem('completedDays') || '[]');
    // eslint-disable-next-line react-hooks/set-state-in-effect
    setCompletedDays(stored);
  }, []);

  if (!isClient) return null;

  const totalDays = 100;
  const isComplete = completedDays.length >= totalDays;

  return (
    <div className="max-w-4xl mx-auto py-12 px-6">
      <h1 className="text-4xl font-black mb-8 bg-gradient-to-r from-blue-400 via-green-400 to-yellow-400 text-transparent bg-clip-text inline-block">
        Your Journey
      </h1>

      {!isComplete ? (
        <div className="bg-zinc-900 border border-zinc-800 rounded-2xl p-8 text-center">
          <div className="w-20 h-20 bg-zinc-800 rounded-full flex items-center justify-center mx-auto mb-6">
            <Lock size={32} className="text-zinc-500" />
          </div>
          <h2 className="text-2xl font-bold text-zinc-100 mb-4">
            Certificate Locked
          </h2>
          <p className="text-zinc-400 mb-8 max-w-lg mx-auto">
            You have completed <span className="text-white font-bold">{completedDays.length}</span> out of {totalDays} days. Keep building and return here when you finish the entire masterclass to claim your certificate!
          </p>
          <div className="w-full bg-zinc-800 rounded-full h-4 mb-8 overflow-hidden">
            <div 
              className="bg-gradient-to-r from-blue-500 to-green-500 h-4 rounded-full transition-all duration-1000"
              style={{ width: `${(completedDays.length / totalDays) * 100}%` }}
            ></div>
          </div>
          <Link href="/" className="inline-block px-6 py-3 bg-indigo-600 hover:bg-indigo-500 text-white rounded-lg font-medium transition-colors">
            Continue Learning
          </Link>
        </div>
      ) : (
        <div className="space-y-8">
          <div className="bg-zinc-900 border border-zinc-800 rounded-2xl p-8 print:hidden">
            <h2 className="text-2xl font-bold text-zinc-100 mb-4 flex items-center gap-3">
              <Award className="text-yellow-400" size={28} />
              Congratulations!
            </h2>
            <p className="text-zinc-400 mb-6">
              You have completed all 100 days of the Arduino Masterclass. Enter your name below to generate your official certificate.
            </p>
            <div className="flex gap-4">
              <input 
                type="text" 
                placeholder="Enter your full name" 
                value={name}
                onChange={(e) => setName(e.target.value)}
                className="flex-1 bg-zinc-800 border border-zinc-700 rounded-lg px-4 py-2 text-white outline-none focus:border-indigo-500"
              />
              <button 
                onClick={() => window.print()}
                className="px-6 py-2 bg-indigo-600 hover:bg-indigo-500 text-white rounded-lg font-medium transition-colors flex items-center gap-2"
              >
                <Printer size={18} />
                Print Certificate
              </button>
            </div>
            <div className="mt-6 flex items-center gap-4 border-t border-zinc-800 pt-6">
              <span className="text-zinc-400 text-sm">Show off your achievement:</span>
              <TwitterShareButton 
                text="I just completed the epic 100 Days of Arduino Masterclass! 🎉 100 hardware projects built from scratch."
                url="https://github.com/paneendrakumar0/100-Projects-with-Arduino"
              />
            </div>
          </div>

          {/* Certificate Container - Styled for printing */}
          <div className="certificate-container bg-white text-zinc-900 p-12 rounded-2xl border-8 border-double border-zinc-200 text-center relative overflow-hidden">
            <div className="absolute top-0 left-0 w-full h-2 bg-gradient-to-r from-blue-500 via-green-500 to-yellow-500"></div>
            
            <Award size={64} className="text-yellow-500 mx-auto mb-6" />
            
            <h1 className="text-5xl font-serif font-bold text-zinc-800 mb-4 tracking-wide uppercase">
              Certificate of Completion
            </h1>
            
            <p className="text-xl text-zinc-600 italic mb-8 font-serif">
              This is to certify that
            </p>
            
            <div className="text-4xl font-bold text-indigo-900 border-b-2 border-zinc-300 pb-2 mb-8 mx-auto max-w-lg min-h-[60px]">
              {name || "[ Your Name Here ]"}
            </div>
            
            <p className="text-lg text-zinc-600 mb-12 max-w-2xl mx-auto leading-relaxed">
              has successfully completed the grueling <strong>100 Days of Arduino Masterclass</strong>, mastering circuits, microcontrollers, control systems, and embedded C++ architecture.
            </p>
            
            <div className="flex justify-between items-end px-12 mt-16">
              <div className="text-center">
                <div className="border-b border-zinc-400 w-48 pb-2 mb-2"></div>
                <p className="text-sm text-zinc-500 font-bold uppercase tracking-wider">Date Achieved</p>
                <p className="text-zinc-800 font-medium">{new Date().toLocaleDateString()}</p>
              </div>
              
              <div className="w-24 h-24 border-4 border-yellow-400 rounded-full flex items-center justify-center text-yellow-600 font-bold rotate-12 bg-yellow-50">
                100 DAYS
              </div>
              
              <div className="text-center">
                <div className="border-b border-zinc-400 w-48 pb-2 mb-2 italic font-serif text-xl">
                  Paneendra Kumar
                </div>
                <p className="text-sm text-zinc-500 font-bold uppercase tracking-wider">Course Creator</p>
              </div>
            </div>
          </div>
        </div>
      )}

      <style jsx global>{`
        @media print {
          body * {
            visibility: hidden;
          }
          .certificate-container, .certificate-container * {
            visibility: visible;
          }
          .certificate-container {
            position: absolute;
            left: 0;
            top: 0;
            width: 100%;
            height: 100%;
            margin: 0;
            border: none;
            box-shadow: none;
          }
        }
      `}</style>
    </div>
  );
}

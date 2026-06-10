'use client';

import { motion, useScroll } from 'framer-motion';

export default function ProgressBar() {
  const { scrollYProgress } = useScroll();
  
  return (
    <motion.div
      style={{
        scaleX: scrollYProgress,
        transformOrigin: "left",
        position: "fixed",
        top: 0,
        left: 0,
        right: 0,
        height: "4px",
        background: "linear-gradient(90deg, #00e5ff, #7000ff)",
        zIndex: 9999,
        boxShadow: "0 0 10px rgba(0, 229, 255, 0.5)"
      }}
    />
  );
}

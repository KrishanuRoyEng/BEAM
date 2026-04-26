import { NextResponse } from 'next/server';

interface Reading {
  b_abs: number;
  r_abs: number;
  tcb: number;
  timestamp: string;
}

// In-memory store for MVP. Note: In serverless environments, this will reset periodically.
// For local development, it persists as long as the server is running.
declare global {
  var latestReading: Reading | undefined;
}

const defaultReading: Reading = {
  b_abs: 0,
  r_abs: 0,
  tcb: 12.5, // Sample data for initial load
  timestamp: new Date().toISOString()
};

export async function GET() {
  const reading = global.latestReading || defaultReading;
  return NextResponse.json(reading);
}

export async function POST(request: Request) {
  try {
    const data = await request.json();
    
    // Basic validation
    if (
      typeof data.b_abs !== 'number' || 
      typeof data.r_abs !== 'number' || 
      typeof data.tcb !== 'number'
    ) {
      return NextResponse.json({ error: 'Invalid data payload structure' }, { status: 400 });
    }

    const newReading: Reading = {
      b_abs: data.b_abs,
      r_abs: data.r_abs,
      tcb: data.tcb,
      timestamp: new Date().toISOString()
    };

    global.latestReading = newReading;

    return NextResponse.json({ 
      success: true, 
      message: 'Reading recorded',
      data: newReading 
    });
  } catch (err) {
    return NextResponse.json({ error: 'Failed to parse JSON or internal error' }, { status: 400 });
  }
}

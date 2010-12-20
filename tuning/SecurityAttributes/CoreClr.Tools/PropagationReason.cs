using System;
using Mono.Cecil;

namespace CoreClr.Tools
{
	public abstract class PropagationReason
	{
		public abstract MethodDefinition MethodThatTaintedMe { get; }
		public abstract string Explanation { get; }
	}

	public class PropagationReasonCall : PropagationReason
	{
		public PropagationReasonCall(MethodDefinition caller, MethodDefinition callee)
		{
			_caller = caller;
			_callee = callee;
		}

		private readonly MethodDefinition _callee;
		private readonly MethodDefinition _caller;

		override public string Explanation
		{
            get { return " calls "; } // +_callee; }
		}

		public override MethodDefinition MethodThatTaintedMe
		{
			get { return _callee; }
		}
	}

	public class PropagationReasonRequiresPrivilegesItself : PropagationReason
	{
		public static PropagationReason Default = new PropagationReasonRequiresPrivilegesItself();

		private PropagationReasonRequiresPrivilegesItself()
		{	
		}

		override public string Explanation
		{
			get { return "requires privileges itself"; }
		}

		public override MethodDefinition MethodThatTaintedMe
		{
			get { return null; }
		}
	}

    public class PropagationReasonIsInSameEnheritanceGraphAs : PropagationReason
    {
        private readonly MethodDefinition _taintedMethod;
        private readonly MethodDefinition _taintingMethod;

        public PropagationReasonIsInSameEnheritanceGraphAs(MethodDefinition taintedMethod, MethodDefinition taintingMethod)
        {
            if (taintedMethod.Equals(taintingMethod)) throw new ArgumentException("Cannot create PropagationReasonIsInSameEnheritanceGraphAs where tainter equals tainted");
            _taintedMethod = taintedMethod;
            _taintingMethod = taintingMethod;
        }

        public override MethodDefinition MethodThatTaintedMe
        {
            get { return _taintingMethod; }
        }

        public override string Explanation
        {
            get { return "Is in same enheritancegraph as " + _taintingMethod; }
        }
    }

	public class PropagationReasonIsInCriticalType : PropagationReason
	{
		public PropagationReasonIsInCriticalType(MethodDefinition method)
		{
			_method = method;
		}

		private MethodDefinition _method;

		public override MethodDefinition MethodThatTaintedMe
		{
			get { return null; }
		}

		public override string Explanation
		{
			get { return "lives in a critical type. (ML:" + Moonlight.GetSecurityStatusFor(_method) + ")"; }
		}
	}

}

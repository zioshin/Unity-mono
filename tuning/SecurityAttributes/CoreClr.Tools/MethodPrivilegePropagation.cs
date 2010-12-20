using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Mono.Cecil;

namespace CoreClr.Tools
{
	public enum PropagationReasonEnum
	{
		Call,
		Override,
		Overriden,
		RequiresPrivilegesItself
	}

	public class MethodPrivilegePropagation
	{
		private readonly AssemblyDefinition[] _assemblies;
		private List<MethodDefinition> _methodRequiringPrivilegesThemselves;
		private HashSet<MethodDefinition> _canBeSscManual;

		private ICollection<MethodDefinition> _resultingCriticalMethods;

		private Dictionary<MethodDefinition, List<PropagationReason>> _propagationGraph;
		private Queue<MethodDefinition> _propagationQueue = new Queue<MethodDefinition>();
		private MethodMap _methodMap;
	    private ICollection<TypeDefinition> _criticalTypes;

		

		public MethodPrivilegePropagation(AssemblyDefinition[] assemblies, IEnumerable<MethodDefinition> methodRequiringPrivileges, IEnumerable<MethodDefinition> sscMethods, ICollection<TypeDefinition> criticalTypes, ICollection<MethodToMethodCall> ignoreCalls)
		{
			_assemblies = assemblies;
            AssemblySetResolver.SetUp(assemblies);
            Analyze(methodRequiringPrivileges, sscMethods, criticalTypes, ignoreCalls);
		}

		public IEnumerable<MethodDefinition> ResultingCriticalMethods
		{
			get { return _resultingCriticalMethods; }
		}

		public MethodPrivilegePropagationReportBuilder CreateReportBuilder()
		{
			return new MethodPrivilegePropagationReportBuilder(_assemblies, _methodRequiringPrivilegesThemselves, _canBeSscManual, _resultingCriticalMethods, _propagationGraph, _criticalTypes);
		}

		private void Analyze(IEnumerable<MethodDefinition> methodRequiringPrivileges, IEnumerable<MethodDefinition> sscMethods, ICollection<TypeDefinition> criticalTypes, ICollection<MethodToMethodCall> ignoreMethodCalls)
		{	
			_methodRequiringPrivilegesThemselves = methodRequiringPrivileges.ToList();
			_canBeSscManual = new HashSet<MethodDefinition>(sscMethods);
		    _resultingCriticalMethods = new List<MethodDefinition>();
			_propagationGraph = new Dictionary<MethodDefinition, List<PropagationReason>>();
			_criticalTypes = criticalTypes;
			_methodMap = new MethodMap(_assemblies, ignoreMethodCalls);

			InitPropagationReasonForCriticalTypeMembers();
			InitPropagationReasonForMethodsRequiringPrivilegesThemselves();
			Propagate();
		}

	    private void Propagate()
		{	
			var processed = new HashSet<MethodDefinition>();
			while (_propagationQueue.Count > 0)
			{
				var method = _propagationQueue.Dequeue();
				if (processed.Contains(method))
					continue;

				processed.Add(method);

			    var ssc = _canBeSscManual.Contains(method);
                var reasons = GetPropagationReasonsFor(method);
                if (ssc)
                {
                    var p = reasons.OfType<PropagationReasonIsInSameEnheritanceGraphAs>().FirstOrDefault();
                    if (p != null)
                    {
                        bool temporarilyDisableSSCWarningWhileInvestigating = false;
                        if (temporarilyDisableSSCWarningWhileInvestigating)
                        {
                            ssc = false;
                        }
                        else
                        {
                            throw new MethodNeedsSCButIsMarkedSSCException(method, p);
                        }
                    }
                    else
                    {
                        if (GetPropagationReasonsFor(method).Single().GetType() ==
                            typeof (PropagationReasonRequiresPrivilegesItself))
                            continue;
                    }
                }

			    if (!_criticalTypes.Contains(method.DeclaringType) && !ssc)
                {
                    if (!reasons.OfType<PropagationReasonIsInSameEnheritanceGraphAs>().Any())
                    {
                        //SC method in Transparent type.  these are fucked up, as they 'pollute' an entire enheritance graph. because of coreclr rules, everything it overrides,
                        //and everything overriding it must also have method level [sc]. If this were .ToString(), that means all .ToString() methods everywhere become [sc].
                        foreach (var m in _methodMap.GetEntireMethodEnheritanceGraph(method))
                        {
                            if (m == method) continue;
                            AddPropagationReasonFor(m, new PropagationReasonIsInSameEnheritanceGraphAs(m, method));
                            _propagationQueue.Enqueue(m);
                        }
                    }
                }

				EnqueueAllTainted(method, (m) => _methodMap.CallersOf(m).Where(taintedmethod => !_canBeSscManual.Contains(taintedmethod)), _propagationQueue, (m) => new PropagationReasonCall(method,m));

                _resultingCriticalMethods.Add(method);
			}
	        
            //now all calling based propagation has happened. we're going to remove the methods that live in criticaltypes,
            //as they are already allowed to call whatever they want. Except those methods that have [sc] because they live in the same entiremethodhierarchy as a transparent
            //type with that method having [sc].

	        var sw = System.Diagnostics.Stopwatch.StartNew();
	        var candidatesForRemoval =_resultingCriticalMethods.Where(m => _criticalTypes.Contains(m.DeclaringType)).ToArray();
	        Console.WriteLine("Check1: " + sw.ElapsedMilliseconds);
            sw.Reset();
	        var canremove = candidatesForRemoval.Where(m =>!GetPropagationReasonsFor(m).Any(r => r.GetType() == typeof (PropagationReasonIsInSameEnheritanceGraphAs))).ToArray();
            
            Console.WriteLine("Check2: " + sw.ElapsedMilliseconds);
            sw.Reset();
            _resultingCriticalMethods = _resultingCriticalMethods.Where(m => !canremove.Contains(m)).ToList();
            Console.WriteLine("Check3: " + sw.ElapsedMilliseconds);
		}

	    private PropagationReason[] FindMethodsInEnheritanceGraphThatHaveReasonsOtherThanBeingInTheGraph(MethodDefinition method)
	    {
	        var graph = _methodMap.GetEntireMethodEnheritanceGraph(method);
	        var result = new List<PropagationReason>();
            foreach(var m in graph)
            {
                var reasons = GetPropagationReasonsFor(m);
                foreach(var reason in reasons.Where(r=>r.GetType() != typeof(PropagationReasonIsInSameEnheritanceGraphAs)))
	            {
	                result.Add(reason);
	            }
            }
	        return result.ToArray();
	    }


	    private void InitPropagationReasonForMethodsRequiringPrivilegesThemselves()
		{
			foreach (var m in _methodRequiringPrivilegesThemselves)
			{
				AddPropagationReasonFor(m, PropagationReasonRequiresPrivilegesItself.Default);
				_propagationQueue.Enqueue(m);
			}

		}

		private void InitPropagationReasonForCriticalTypeMembers()
		{
			foreach (var m in _criticalTypes.SelectMany(t => t.AllMethodsAndConstructors()))
			{
			    AddPropagationReasonFor(m, new PropagationReasonIsInCriticalType(m));
				_propagationQueue.Enqueue(m);
			}
		}

		private void EnqueueAllTainted(MethodDefinition method, Func<MethodDefinition, IEnumerable<MethodDefinition>> whichTaints, Queue<MethodDefinition> queue, Func<MethodDefinition, PropagationReason> reasonCreator)
		{
			foreach (var tainted in whichTaints(method))
			{
				if (!_criticalTypes.Contains(tainted.DeclaringType))
				{
					var propagationReason = reasonCreator(method);
					AddPropagationReasonFor(tainted, propagationReason);
				}
				queue.Enqueue(tainted);
			}
		}

	    private void AddPropagationReasonFor(MethodDefinition method, PropagationReason reason)
		{
			if (!_propagationGraph.ContainsKey(method))
				_propagationGraph[method] = new List<PropagationReason>();

            _propagationGraph[method].Add(reason);
		}
		IEnumerable<PropagationReason> GetPropagationReasonsFor(MethodDefinition method)
		{
			if (!_propagationGraph.ContainsKey(method)) return null;
			return _propagationGraph[method];
		}
	}

    public class MethodNeedsSCButIsMarkedSSCException : Exception
    {
        private readonly MethodDefinition _method;
        private readonly PropagationReason _propagationReason;

        public MethodNeedsSCButIsMarkedSSCException(MethodDefinition method, PropagationReasonIsInSameEnheritanceGraphAs propagationReason)
        {
            _method = method;
            _propagationReason = propagationReason;
        }
        public override string ToString()
        {
            var sb = new StringBuilder("Method: " + _method +
                   " is marked as SSC, but its enheritance chain needs to be SC because it is in the same graph as: "+_propagationReason.MethodThatTaintedMe);
            
            return sb.ToString();
        }
    }

    public class MethodToMethodCall
	{
		public MethodToMethodCall(MethodDefinition caller, MethodDefinition callee)
		{
			Callee = callee;
			Caller = caller;
		}

		MethodDefinition Caller { get; set; }
		MethodDefinition Callee { get; set; }
		
		public override bool Equals(object obj)
		{
			var other = obj as MethodToMethodCall;
			if (other==null) return false;
			return Caller.Equals(other.Caller) && Callee.Equals(other.Callee);
		}
	}

}
